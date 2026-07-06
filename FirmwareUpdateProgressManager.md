# IRIS Kit Firmware Update - Embedded Device Interface Specification

## Overview

This document describes the MQTT interface and implementation requirements for IRIS Kit (ESP8266-based) embedded devices to support remote firmware updates with progress tracking via OTA (Over-The-Air).

**Hardware Platform:** ESP8266 (ESP-01M)  
**Framework:** Arduino  
**MQTT Library:** Aliyun IoT SDK / EMQX MQTT Client  
**OTA Method:** ESP8266HTTPUpdate (HTTP-based OTA)

---

## Architecture

```
Java Backend (IrisIoTService)
     ↓ MQTT: __firmware_update
MQTT Broker (Aliyun IoT / EMQX)
     ↓ Topic: /{productKey}/{deviceName}/user/iris_kit_downstream
IRIS Kit (ESP8266)
     ├─ iris_client.cpp → handleFirmwareUpdate()
     ├─ ota_manager.cpp → startOTAUpgrade()
     └─ HTTP Download → ESPhttpUpdate.update()
     ↓ MQTT: __firmware_update_progress
MQTT Broker
     ↑ Topic: /{productKey}/{deviceName}/user/iris_kit_upstream
Java Backend → SSE Stream → Node.js → Browser
```

---

## 1. MQTT Topics

### Downstream Topic (Server → Device)

**Format:** `/{productKey}/{deviceName}/user/iris_kit_downstream`

**Example:** 
- Development: `/a1WlzsJh50b/kit_001/user/iris_kit_downstream_dev`
- Production: `/a1ihYt1lqGH/kit_001/user/iris_kit_downstream`

### Upstream Topic (Device → Server)

**Format:** `/{productKey}/{deviceName}/user/iris_kit_upstream`

**Example:**
- Development: `/a1WlzsJh50b/kit_001/user/iris_kit_upstream_dev`
- Production: `/a1ihYt1lqGH/kit_001/user/iris_kit_upstream`

---

## 2. Firmware Update Command (Downstream)

### Message Structure

When the server initiates a firmware update, it publishes a command message to the downstream topic.

**Event Name:** `__firmware_update`

**JSON Payload:**
```json
{
  "eventName": "__firmware_update",
  "productKey": "a1WlzsJh50b",
  "deviceName": "kit_001",
  "content": "{\"firmware_version\":\"1.6.0\",\"version_code\":6,\"download_url\":\"http://irext-debug.oss-cn-hangzhou.aliyuncs.com/irda_12345.bin\"}"
}
```

### Field Descriptions

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| eventName | String | Yes | Always `"__firmware_update"` |
| productKey | String | Yes | Device product key (must match device config) |
| deviceName | String | Yes | Device unique name (must match device config) |
| content | String | Yes | JSON-encoded string with firmware metadata |

### Content Field Structure

The `content` field is a **JSON-encoded string** (not a nested object) containing:

```json
{
  "firmware_version": "1.6.0",
  "version_code": 6,
  "download_url": "http://irext-debug.oss-cn-hangzhou.aliyuncs.com/irda_12345.bin"
}
```

| Field | Type | Description |
|-------|------|-------------|
| firmware_version | String | Target firmware version (e.g., "1.6.0") |
| version_code | Integer | Numeric version code for comparison |
| download_url | String | HTTP URL to download firmware .bin file |

**Note:** The download URL typically points to OSS (Object Storage Service) or any HTTP server hosting the firmware binary.

---

## 3. Progress Reporting (Upstream)

The device reports OTA progress by publishing messages to the upstream topic using the existing MQTT infrastructure.

### Message Structure

**Event Name:** `__firmware_update_progress`

**JSON Payload:**
```json
{
  "eventName": "__firmware_update_progress",
  "productKey": "a1WlzsJh50b",
  "deviceName": "kit_001",
  "appId": 123,
  "consoleId": 0,
  "remoteIndex": "",
  "keyId": 0,
  "keyName": "",
  "resp": "ota_progress",
  "payload": "{\"stage\":\"downloading\",\"progress\":45,\"message\":\"Downloading: 45%\",\"success\":true}"
}
```

### Field Descriptions

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| eventName | String | Yes | Always `"__firmware_update_progress"` |
| productKey | String | Yes | Device product key |
| deviceName | String | Yes | Device unique name |
| appId | Integer | Yes | Application ID from device authentication |
| consoleId | Integer | No | Console session ID (use 0 for OTA) |
| remoteIndex | String | No | Remote index (use empty string for OTA) |
| keyId | Integer | No | Key ID (use 0 for OTA) |
| keyName | String | No | Key name (use empty string for OTA) |
| resp | String | Yes | Response type: `"ota_progress"` |
| payload | String | Yes | JSON-encoded string with progress details |

### Payload Field Structure

The `payload` field is a **JSON-encoded string** containing:

```json
{
  "stage": "downloading",
  "progress": 45,
  "message": "Downloading: 45%",
  "success": true
}
```

| Field | Type | Description |
|-------|------|-------------|
| stage | String | Current OTA stage (see below) |
| progress | Integer | Progress percentage (0-100) |
| message | String | Human-readable status message |
| success | Boolean | Whether operation is successful |

### OTA Stages

| Stage | Progress Range | Description |
|-------|---------------|-------------|
| `idle` | 0% | Initial state, waiting for command |
| `downloading` | 1-99% | Downloading firmware via HTTP |
| `completed` | 100% | Download completed successfully |
| `failed` | 0% | Update failed at any stage |

**Note:** Unlike the generic specification, ESP8266 HTTP OTA handles flash writing internally, so there's no separate "writing_flash" stage. The progress only covers the download phase.

---

## 4. Implementation Details

### Current Code Structure

The IRIS Kit firmware already has the following components:

1. **iris_client.cpp** - MQTT message handler
   - `handleFirmwareUpdate()` function at line 542
   - Event handler table includes `EVENT_NAME_FIRMWARE_UPDATE`

2. **ota_manager.cpp** - OTA management
   - `startOTAUpgrade()` - Initiates HTTP OTA
   - `reportOTAStatus()` - Reports status (needs MQTT integration)
   - HTTP update callbacks: `onStart`, `onProgress`, `onEnd`

3. **iot_hub.cpp** - MQTT communication
   - `sendData()` - Publishes to upstream topic

### Integration Steps

#### Step 1: Parse Firmware Update Command

In `iris_client.cpp`, the `handleFirmwareUpdate()` function should:

```cpp
static int handleFirmwareUpdate(String product_key, String device_name, String content) {
    if (product_key.isEmpty() || device_name.isEmpty()) {
        ERROR("Invalid product key or device name\n");
        return -1;
    }
    
    // Verify product key and device name match
    if (!product_key.equals(g_product_key) || !device_name.equals(g_device_name)) {
        ERROR("Product key or device name mismatch\n");
        return -1;
    }
    
    INFOF("Received firmware_update command, content = %s\n", content.c_str());
    
    // Parse content JSON
    StaticJsonDocument<512> doc;
    if (DeserializationError::Ok != deserializeJson(doc, content)) {
        ERROR("Failed to parse firmware update content\n");
        return -1;
    }
    
    String firmware_url = doc["download_url"];
    String target_version = doc["firmware_version"];
    int version_code = doc["version_code"];
    
    if (firmware_url.isEmpty()) {
        ERROR("Download URL is empty\n");
        return -1;
    }
    
    // Start OTA upgrade
    int ret = startOTAUpgrade(firmware_url, target_version, version_code);
    
    return ret;
}
```

#### Step 2: Report OTA Progress via MQTT

In `ota_manager.cpp`, implement `reportOTAStatus()` to send MQTT messages:

```cpp
void reportOTAStatus(ota_status_t status, const String& message) {
    INFOF("OTA Status: %d, Message: %s\n", status, message.c_str());
    
    // Build progress payload
    StaticJsonDocument<512> payload_doc;
    
    // Determine stage and progress based on status
    String stage;
    int progress = 0;
    bool success = true;
    
    switch (status) {
        case OTA_STATUS_IDLE:
            stage = "idle";
            progress = 0;
            break;
        case OTA_STATUS_DOWNLOADING:
            stage = "downloading";
            progress = current_download_percent; // Track this in httpUpdateProgress
            break;
        case OTA_STATUS_SUCCESS:
            stage = "completed";
            progress = 100;
            break;
        case OTA_STATUS_FAILED:
            stage = "failed";
            progress = 0;
            success = false;
            break;
        case OTA_STATUS_REBOOTING:
            stage = "rebooting";
            progress = 95;
            break;
    }
    
    payload_doc["stage"] = stage;
    payload_doc["progress"] = progress;
    payload_doc["message"] = message;
    payload_doc["success"] = success;
    
    String payload_str;
    serializeJson(payload_doc, payload_str);
    
    // Build upstream message
    mqtt_upstream_topic_msg_doc.clear();
    mqtt_upstream_topic_msg_doc["eventName"] = "__firmware_update_progress";
    mqtt_upstream_topic_msg_doc["productKey"] = g_product_key;
    mqtt_upstream_topic_msg_doc["deviceName"] = g_device_name;
    mqtt_upstream_topic_msg_doc["appId"] = g_app_id;
    mqtt_upstream_topic_msg_doc["consoleId"] = 0;
    mqtt_upstream_topic_msg_doc["remoteIndex"] = "";
    mqtt_upstream_topic_msg_doc["keyId"] = 0;
    mqtt_upstream_topic_msg_doc["keyName"] = "";
    mqtt_upstream_topic_msg_doc["resp"] = "ota_progress";
    mqtt_upstream_topic_msg_doc["payload"] = payload_str;
    
    String mqtt_message;
    serializeJson(mqtt_upstream_topic_msg_doc, mqtt_message);
    
    // Send to upstream topic
    sendData(g_upstream_topic.c_str(), (uint8_t*)mqtt_message.c_str(), mqtt_message.length());
    
    // Call user callback if set
    if (ota_callback != nullptr) {
        ota_callback(status, message.c_str());
    }
}
```

#### Step 3: Track Download Progress

Add a global variable to track download percentage:

```cpp
// In ota_manager.cpp
static int current_download_percent = 0;

static void httpUpdateProgress(int current, int total) {
    int percent = (total > 0) ? (current * 100 / total) : 0;
    
    // Only report every 5% to avoid MQTT flooding
    if (percent % 5 == 0 && percent != current_download_percent) {
        current_download_percent = percent;
        
        char msg[64];
        snprintf(msg, sizeof(msg), "Downloading: %d%%", percent);
        reportOTAStatus(OTA_STATUS_DOWNLOADING, String(msg));
        
        INFOF("HTTP update progress: %d%% (%d/%d bytes)\n", percent, current, total);
    }
}
```

---

## 5. Complete Message Flow Example

### Scenario: Successful Firmware Update

#### 1. Server Sends Update Command

**Topic:** `/a1WlzsJh50b/kit_001/user/iris_kit_downstream_dev`

**Payload:**
```json
{
  "eventName": "__firmware_update",
  "productKey": "a1WlzsJh50b",
  "deviceName": "kit_001",
  "content": "{\"firmware_version\":\"1.6.0\",\"version_code\":6,\"download_url\":\"http://irext-debug.oss-cn-hangzhou.aliyuncs.com/irda_12345.bin\"}"
}
```

#### 2. Device Reports Progress

**Topic:** `/a1WlzsJh50b/kit_001/user/iris_kit_upstream_dev`

**Progress Messages:**

```json
// Initial download
{
  "eventName": "__firmware_update_progress",
  "productKey": "a1WlzsJh50b",
  "deviceName": "kit_001",
  "appId": 123,
  "consoleId": 0,
  "remoteIndex": "",
  "keyId": 0,
  "keyName": "",
  "resp": "ota_progress",
  "payload": "{\"stage\":\"downloading\",\"progress\":5,\"message\":\"Downloading: 5%\",\"success\":true}"
}
```

```json
// Mid-download
{
  "eventName": "__firmware_update_progress",
  "productKey": "a1WlzsJh50b",
  "deviceName": "kit_001",
  "appId": 123,
  "consoleId": 0,
  "remoteIndex": "",
  "keyId": 0,
  "keyName": "",
  "resp": "ota_progress",
  "payload": "{\"stage\":\"downloading\",\"progress\":50,\"message\":\"Downloading: 50%\",\"success\":true}"
}
```

```json
// Download complete
{
  "eventName": "__firmware_update_progress",
  "productKey": "a1WlzsJh50b",
  "deviceName": "kit_001",
  "appId": 123,
  "consoleId": 0,
  "remoteIndex": "",
  "keyId": 0,
  "keyName": "",
  "resp": "ota_progress",
  "payload": "{\"stage\":\"completed\",\"progress\":100,\"message\":\"Download completed, rebooting...\",\"success\":true}"
}
```

#### 3. Device Reboots

After sending the completion message, the device waits 1 second and calls `ESP.restart()`.

---

## 6. Error Handling

### Failed Update Example

If the download fails (network error, invalid URL, etc.):

```json
{
  "eventName": "__firmware_update_progress",
  "productKey": "a1WlzsJh50b",
  "deviceName": "kit_001",
  "appId": 123,
  "consoleId": 0,
  "remoteIndex": "",
  "keyId": 0,
  "keyName": "",
  "resp": "ota_progress",
  "payload": "{\"stage\":\"failed\",\"progress\":0,\"message\":\"HTTP_UPDATE_FAILED: (4) Connection timeout\",\"success\":false}"
}
```

### Common Error Scenarios

| Error | Cause | Solution |
|-------|-------|----------|
| HTTP_UPDATE_FAILED | Network timeout, invalid URL | Retry with exponential backoff |
| HTTP_UPDATE_NO_UPDATES | Firmware version is same or older | Skip update, report success |
| Product key mismatch | Command targeted wrong device | Reject command, log error |
| Empty download URL | Malformed command | Reject command, report error |

---

## 7. MQTT QoS and Timing

### QoS Settings

- **Downstream (Command):** QoS 1 (at least once)
- **Upstream (Progress):** QoS 0 (at most once, acceptable to lose intermediate updates)

### Reporting Frequency

- **Recommended:** Every 5% progress change
- **Minimum:** At stage transitions (start, complete, failed)
- **Maximum:** Avoid reporting more than once per second

### Reboot Timing

After sending the final "completed" message:
```cpp
delay(1000);  // Wait 1 second for MQTT delivery
ESP.restart();
```

---

## 8. Testing Checklist

Before deployment, verify:

- [ ] Device correctly parses `__firmware_update` command
- [ ] Product key and device name validation works
- [ ] Download URL is extracted correctly from content JSON
- [ ] Progress messages are sent every 5%
- [ ] MQTT messages use correct upstream topic format
- [ ] Final "completed" message is sent before reboot
- [ ] Failed updates report error message with success=false
- [ ] Device reboots after successful download
- [ ] New firmware boots successfully after restart
- [ ] Rollback mechanism works if new firmware fails to boot

---

## 9. Code Integration Summary

### Files to Modify

1. **iris_client.cpp**
   - Update `handleFirmwareUpdate()` to parse content and call `startOTAUpgrade()`

2. **ota_manager.cpp**
   - Implement `reportOTAStatus()` to send MQTT messages
   - Add `current_download_percent` tracking variable
   - Update `httpUpdateProgress()` to report via MQTT

3. **global.h** (if needed)
   - Add JSON document for OTA progress messages

### Existing Infrastructure

The following components are already implemented:

- ✅ MQTT connection (Aliyun IoT / EMQX)
- ✅ Event handler table in `iris_client.cpp`
- ✅ `EVENT_NAME_FIRMWARE_UPDATE` constant defined
- ✅ `sendData()` function in `iot_hub.cpp`
- ✅ HTTP OTA callbacks in `ota_manager.cpp`
- ✅ JSON document pool in `global.h`

---

## 10. Reference Implementation

### Minimal Working Example

```cpp
// In iris_client.cpp
static int handleFirmwareUpdate(String product_key, String device_name, String content) {
    if (product_key.isEmpty() || device_name.isEmpty()) {
        ERROR("Invalid product key or device name\n");
        return -1;
    }
    
    if (!product_key.equals(g_product_key) || !device_name.equals(g_device_name)) {
        ERROR("Product key or device name mismatch\n");
        return -1;
    }
    
    INFOF("Received firmware_update command\n");
    
    StaticJsonDocument<512> doc;
    if (DeserializationError::Ok != deserializeJson(doc, content)) {
        ERROR("Failed to parse firmware update content\n");
        return -1;
    }
    
    String firmware_url = doc["download_url"];
    String target_version = doc["firmware_version"];
    int version_code = doc["version_code"];
    
    if (firmware_url.isEmpty()) {
        ERROR("Download URL is empty\n");
        return -1;
    }
    
    return startOTAUpgrade(firmware_url, target_version, version_code);
}
```

---

## 11. Troubleshooting

### Issue: Progress messages not received

**Check:**
1. MQTT connection is active before sending
2. Upstream topic format is correct
3. JSON payload is properly formatted
4. Use MQTT client tool to subscribe and verify messages

### Issue: OTA fails with HTTP_UPDATE_FAILED

**Check:**
1. Download URL is accessible from device network
2. Firmware .bin file is valid ESP8266 binary
3. Sufficient flash space available
4. WiFi signal strength is adequate

### Issue: Device bricks after update

**Solution:**
1. ESP8266 HTTP OTA uses safe partition switching
2. If new firmware crashes, watchdog will reset
3. Implement manual rollback via physical button
4. Keep backup firmware in secondary partition

---

## Contact

For technical support or questions about this interface specification, please contact the IRIS Server development team.

**Document Version:** 1.0  
**Last Updated:** 2026-06-20  
**Compatible Firmware:** IRIS Kit v1.5.2+
