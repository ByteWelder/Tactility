#include "FT6X36.h"

#include "freertos/FreeRTOS.h"

#define CONFIG_FT6X36_DEBUG false

FT6X36 *FT6X36::_instance = nullptr;
static const char *TAG = "i2c-touch";

//Handle indicating I2C is ready to read the touch
SemaphoreHandle_t TouchSemaphore = xSemaphoreCreateBinary();

FT6X36::FT6X36(i2c_port_t port, gpio_num_t interruptPin)
{
    _instance = this;
    _port = port;
	_intPin = interruptPin;
}

// Destructor should detach interrupt to the pin
FT6X36::~FT6X36()
{
	if (_intPin >= 0)
		gpio_isr_handler_remove((gpio_num_t)_intPin);
}

bool FT6X36::begin(uint8_t threshold, uint16_t width, uint16_t height)
{
	_touch_width = width;
	_touch_height = height;
	if (width == 0 || height ==0) {
		ESP_LOGE(TAG,"begin(uint8_t threshold, uint16_t width, uint16_t height) did not receive the width / height so touch cannot be rotation aware");
	}

	uint8_t data_panel_id;
	readRegister8(FT6X36_REG_PANEL_ID, &data_panel_id);

	if (data_panel_id != FT6X36_VENDID) {
		ESP_LOGE(TAG,"FT6X36_VENDID does not match. Received:0x%x Expected:0x%x\n",data_panel_id,FT6X36_VENDID);
		return false;
		}
		ESP_LOGI(TAG, "\tDevice ID: 0x%02x", data_panel_id);

	uint8_t chip_id;
	readRegister8(FT6X36_REG_CHIPID, &chip_id);
	if (chip_id != FT6206_CHIPID && chip_id != FT6236_CHIPID && chip_id != FT6336_CHIPID) {
		ESP_LOGE(TAG,"FT6206_CHIPID does not match. Received:0x%x\n",chip_id);
		return false;
	}
	ESP_LOGI(TAG, "\tFound touch controller with Chip ID: 0x%02x", chip_id);

	if (_intPin >= 0)
	{
        // INT pin triggers the callback function on the Falling edge of the GPIO
        gpio_config_t io_conf;
        io_conf.intr_type = GPIO_INTR_NEGEDGE; // GPIO_INTR_NEGEDGE repeats always interrupt
        io_conf.pin_bit_mask = 1ULL<<_intPin;  
        io_conf.mode = GPIO_MODE_INPUT;
        io_conf.pull_down_en = (gpio_pulldown_t) 0; // disable pull-down mode
        io_conf.pull_up_en   = (gpio_pullup_t) 1;   // pull-up mode
        gpio_config(&io_conf);

        esp_err_t isr_service = gpio_install_isr_service(0);
        printf("ISR trigger install response: 0x%x %s\n", isr_service, (isr_service==0)?"ESP_OK":"");
        gpio_isr_handler_add((gpio_num_t)_intPin, isr, (void*) 1);
	}

	writeRegister8(FT6X36_REG_DEVICE_MODE, 0x00);
	writeRegister8(FT6X36_REG_THRESHHOLD, threshold);
	writeRegister8(FT6X36_REG_TOUCHRATE_ACTIVE, 0x0E);
	return true;
}

void FT6X36::registerTouchHandler(void (*fn)(TPoint point, TEvent e))
{
	_touchHandler = fn;
	if (CONFIG_FT6X36_DEBUG) printf("Touch handler function registered\n");
}

uint8_t FT6X36::touched()
{
	uint8_t data_buf;
    esp_err_t ret = readRegister8(FT6X36_REG_NUM_TOUCHES, &data_buf);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Error reading from device: %s", esp_err_to_name(ret));
	 }

	if (data_buf > 2)
	{
		data_buf = 0;
	}

	return data_buf;
}

void FT6X36::loop()
{
	processTouch();
}

void IRAM_ATTR FT6X36::isr(void* arg)
{
	/* Un-block the interrupt processing task now */
    xSemaphoreGive(TouchSemaphore);
	//xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void FT6X36::processTouch()
{
	/* Task move to Block state to wait for interrupt event */
    if (_intPin >= 0)
    {
    	if (xSemaphoreTake(TouchSemaphore, portMAX_DELAY) == false) return;
    }

	readData();
	uint8_t n = 0;
	TRawEvent event = (TRawEvent)_touchEvent[n];
	TPoint point{_touchX[n], _touchY[n]};

	switch (event) {

		case TRawEvent::PressDown:
				_points[0] = point;
				_dragMode = false;
				// Note: Is in microseconds. Ref https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/esp_timer.html
				_touchStartTime = esp_timer_get_time()/1000;
				fireEvent(point, TEvent::TouchStart);
			break;

		case TRawEvent::Contact:
				// Dragging makes no sense IMHO. Since the X & Y are not getting updated while dragging
				// Dragging && _points[0].aboutEqual(point) - Not used IDEA 2:  && (lastEvent == 2)
				if (!_dragMode && 
					 (abs(lastX-_touchX[n]) <= maxDeviation || abs(lastY-_touchY[n])<=maxDeviation) && 
				     esp_timer_get_time()/1000 - _touchStartTime > 300) {
					_dragMode = true;
					fireEvent(point, TEvent::DragStart);
					#if defined(CONFIG_FT6X36_DEBUG_EVENTS) && CONFIG_FT6X36_DEBUG_EVENTS==1
						printf("EV: DragStart\n");
					#endif
					
				} else if (_dragMode) {
					fireEvent(point, TEvent::DragMove);
					#if defined(CONFIG_FT6X36_DEBUG_EVENTS) && CONFIG_FT6X36_DEBUG_EVENTS==1
						printf("EV: DragMove\n");
					#endif
				}
				fireEvent(point, TEvent::TouchMove);

			   // For me the _touchStartTime shouold be set in both PressDown & Contact events, but after Drag detection
		        _touchStartTime = esp_timer_get_time()/1000;
				break;

		case TRawEvent::LiftUp:
			
			_points[9] = point;
			_touchEndTime = esp_timer_get_time()/1000;

			//printf("TIMEDIFF: %lu End: %lu\n", _touchEndTime - _touchStartTime, _touchEndTime);

			fireEvent(point, TEvent::TouchEnd);
			if (_dragMode) {
				fireEvent(point, TEvent::DragEnd);
				#if defined(CONFIG_FT6X36_DEBUG_EVENTS) && CONFIG_FT6X36_DEBUG_EVENTS==1
					printf("EV: DragEnd\n");
				#endif
				_dragMode = false;
			}
		
			if ( _touchEndTime - _touchStartTime <= 900) {
				// Do not get why this: _points[0].aboutEqual(point) (Original library)
				fireEvent(point, TEvent::Tap);
				_points[0] = {0, 0};
				_touchStartTime = 0;

				#if defined(CONFIG_FT6X36_DEBUG_EVENTS) && CONFIG_FT6X36_DEBUG_EVENTS==1
					printf("EV: Tap\n");
				#endif
				_dragMode = false;
			}
			
			break;

			case TRawEvent::NoEvent:
			  #if defined(CONFIG_FT6X36_DEBUG_EVENTS) && CONFIG_FT6X36_DEBUG_EVENTS==1
					printf("EV: NoEvent\n");
		      #endif
			break;
	}
	// Store lastEvent
	lastEvent = (int) event;
	lastX = _touchX[0];
	lastY = _touchY[0];
}

void FT6X36::poll(TPoint * point, TEvent * e)
{
	readData();
	// TPoint point{_touchX[0], _touchY[0]};
	TRawEvent event = (TRawEvent)_touchEvent[0];

	if (point != NULL)
	{
		point->x = _touchX[0];
		point->y = _touchY[0];
	}
	if (e != NULL)
	{
		switch (event)
		{
			case TRawEvent::PressDown:
				*e = TEvent::TouchStart;
				break;
			case TRawEvent::Contact:
				*e = TEvent::TouchMove;
				break;
			case TRawEvent::LiftUp:
			default:
				*e = TEvent::TouchEnd;
				break;
		}
	}
}

uint8_t FT6X36::read8(uint8_t regName) {
	uint8_t buf;
	readRegister8(regName, &buf);
	return buf;
}

#define data_size 16     // Discarding last 2: 0x0E & 0x0F as not relevant
bool FT6X36::readData(void)
{
    esp_err_t ret;
    uint8_t data[data_size];
    uint8_t touch_pnt_cnt;      // Number of detected touch points
    readRegister8(FT6X36_REG_NUM_TOUCHES, &touch_pnt_cnt);

    // Read data
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (FT6X36_ADDR<<1), ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    if (ret != ESP_OK) {
        return ret;
    }

    cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (FT6X36_ADDR<<1)|1, ACK_CHECK_EN);
    i2c_master_read(cmd, data, data_size,  I2C_MASTER_LAST_NACK);
    i2c_master_stop(cmd);
    ret = i2c_master_cmd_begin(_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

	if (CONFIG_FT6X36_DEBUG) {
		//printf("REGISTERS:\n");
		for (int16_t i = 0; i < data_size; i++)
		{
			printf("%x:%x ", i, data[i]);
		}
		printf("\n");
	}

    const uint8_t addrShift = 6;
	
	// READ X, Y and Touch events (X 2)
	for (uint8_t i = 0; i < 2; i++)
	{
		_touchX[i] = data[FT6X36_REG_P1_XH + i * addrShift] & 0x0F;
		_touchX[i] <<= 8;
		_touchX[i] |= data[FT6X36_REG_P1_XL + i * addrShift];
		_touchY[i] = data[FT6X36_REG_P1_YH + i * addrShift] & 0x0F;
		_touchY[i] <<= 8;
		_touchY[i] |= data[FT6X36_REG_P1_YL + i * addrShift];
		_touchEvent[i] = data[FT6X36_REG_P1_XH + i * addrShift] >> 6;
	}
	
	// Make _touchX[idx] and _touchY[idx] rotation aware
	switch (_rotation)
  {
	case 1:
	    swap(_touchX[0], _touchY[0]);
		swap(_touchX[1], _touchY[1]);
		_touchY[0] = _touch_width - _touchY[0] -1;
		_touchY[1] = _touch_width - _touchY[1] -1;
		break;
	case 2:
		_touchX[0] = _touch_width - _touchX[0] - 1;
		_touchX[1] = _touch_width - _touchX[1] - 1;
		_touchY[0] = _touch_height - _touchY[0] - 1;
		_touchY[1] = _touch_height - _touchY[1] - 1;
		break;
	case 3:
		swap(_touchX[0], _touchY[0]);
		swap(_touchX[1], _touchY[1]);
		_touchX[0] = _touch_height - _touchX[0] - 1;
		_touchX[1] = _touch_height - _touchX[1] - 1;
		break;
  }
	if (CONFIG_FT6X36_DEBUG) {
	  printf("X0:%d Y0:%d EVENT:%d\n", _touchX[0], _touchY[0], _touchEvent[0]);
	  //printf("X1:%d Y1:%d EVENT:%d\n", _touchX[1], _touchY[1], _touchEvent[1]);
	}
	return true;
}

void FT6X36::writeRegister8(uint8_t reg, uint8_t value)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
	i2c_master_write_byte(cmd, FT6X36_ADDR << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg , ACK_CHECK_EN);
	i2c_master_write_byte(cmd, value , ACK_CHECK_EN);
	i2c_master_stop(cmd);
    i2c_master_cmd_begin(_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
}

uint8_t FT6X36::readRegister8(uint8_t reg, uint8_t *data_buf)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
	i2c_master_write_byte(cmd, FT6X36_ADDR << 1 | I2C_MASTER_WRITE, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg, I2C_MASTER_ACK);
	// Research: Why it's started a 2nd time here
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (FT6X36_ADDR << 1) | I2C_MASTER_READ, true);

    i2c_master_read_byte(cmd, data_buf, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(_port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

	
	//FT6X36_REG_GESTURE_ID. Check if it can be read!
#if defined(FT6X36_DEBUG) && FT6X36_DEBUG==1
	printf("REG 0x%x: 0x%x\n",reg,ret);
#endif

	return ret;
}

void FT6X36::fireEvent(TPoint point, TEvent e)
{
	if (_touchHandler)
		_touchHandler(point, e);
}

void FT6X36::debugInfo()
{
	printf("            TH_DIFF: %d             CTRL: %d\n", read8(FT6X36_REG_FILTER_COEF), read8(FT6X36_REG_CTRL));
	printf("   TIMEENTERMONITOR: %d     PERIODACTIVE: %d\n", read8(FT6X36_REG_TIME_ENTER_MONITOR), read8(FT6X36_REG_TOUCHRATE_ACTIVE));
	printf("      PERIODMONITOR: %d     RADIAN_VALUE: %d\n", read8(FT6X36_REG_TOUCHRATE_MONITOR), read8(FT6X36_REG_RADIAN_VALUE));
	printf("  OFFSET_LEFT_RIGHT: %d   OFFSET_UP_DOWN: %d\n", read8(FT6X36_REG_OFFSET_LEFT_RIGHT), read8(FT6X36_REG_OFFSET_UP_DOWN));
	printf("DISTANCE_LEFT_RIGHT: %d DISTANCE_UP_DOWN: %d\n", read8(FT6X36_REG_DISTANCE_LEFT_RIGHT), read8(FT6X36_REG_DISTANCE_UP_DOWN));
	printf("      DISTANCE_ZOOM: %d           CIPHER: %d\n", read8(FT6X36_REG_DISTANCE_ZOOM), read8(FT6X36_REG_CHIPID));
	printf("             G_MODE: %d         PWR_MODE: %d\n", read8(FT6X36_REG_INTERRUPT_MODE), read8(FT6X36_REG_POWER_MODE));
	printf("             FIRMID: %d     FOCALTECH_ID: %d     STATE: %d\n", read8(FT6X36_REG_FIRMWARE_VERSION), read8(FT6X36_REG_PANEL_ID), read8(FT6X36_REG_STATE));
}

void FT6X36::setRotation(uint8_t rotation) {
	_rotation = rotation;
}

void FT6X36::setTouchWidth(uint16_t width) {
	printf("touch w:%d\n",width);
	_touch_width = width;
}

void FT6X36::setTouchHeight(uint16_t height) {
	printf("touch h:%d\n",height);
	_touch_height = height;
}
