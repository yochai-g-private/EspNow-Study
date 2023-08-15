/*
 Name:		MenuTool.ino
 Created:	3/29/2020 12:03:48 AM
 Author:	MCP
*/

#include "Menu.Serial.h"

#include <IRremote.h>
#include <LiquidCrystal_I2C.h>

#include "StdIR.h"
#include "MicroController.h"
#include "RedGreenLed.h"
#include "RGB_Led.h"
#include "TimeEx.h"
#include "Location.h"
#include "Templates.h"
#include "Toggler.h"

using namespace NYG;

//************************************************
//	Pins
//************************************************

enum
{
	PIN_IR				= 2,
	PIN_RGB_LED_RED		= 4,
	PIN_RGB_LED_BLUE	= 5,
	PIN_RGB_LED_GREEN	= 6,
	PIN_RG_LED_RED		= A1,
	PIN_RG_LED_GREEN	= A0,
#ifdef ARDUINO_AVR_MEGA2560
	PIN_SDA				= 20,
	PIN_SCL				= 21,
#else
	PIN_SDA				= A4,
	PIN_SCL				= A5,
#endif //ARDUINO_AVR_MEGA2560
};

//************************************************
//	Defines
//************************************************

#define LCD_ADDRESS				0x27
#define LCD_LINE_LEN			16
#define LCD_N_ROWS				2
#define MAXLEN					(LCD_LINE_LEN-1)
#define MAXIDX					(MAXLEN-1)
#define DATE_LEN				10
#define TIME_LEN				8
#define HOUR_AND_MINUTE_LEN		5

enum { DEFAULT_INPUT_TIMEOUT = DEFAULT_READ_FROM_STREAM_TIMEOUT };

#define LED_BLINK_LEN		200

#define LCD_TEXT_LINE	0
#define LCD_VALUE_LINE	1

//************************************************
//	Components
//************************************************

static IRrecv				receiver(PIN_IR);
static LiquidCrystal_I2C	lcd(LCD_ADDRESS, LCD_LINE_LEN, LCD_N_ROWS);
static RedGreenLed			led(PIN_RG_LED_RED, PIN_RG_LED_GREEN);
static RGB_Led				RGB_led(PIN_RGB_LED_RED, PIN_RGB_LED_GREEN, PIN_RGB_LED_BLUE);

//************************************************
//	Variables
//************************************************

static SerialMenu::Response		response;
static String					value;
static StdIR::Key				key;

static Toggler					standby_toggler;

//************************************************
//	Predeclarations
//************************************************

static void run_menu();
static bool treat_response();
static bool update_value();
static bool display_text();
static bool navigate();
static bool get_navigation_input(SerialMenu::NavigationDirection& direction, uint8_t& child_id);
static bool set_value();
static bool enterSignedReal(double minval, double maxval, int decimal_digits, double resolution = 0);
static bool enterDateTime(char* s, const int SLEN);
static bool enterBoolean(const char* ON, const char* OFF);
static void onBadInput();
static void onGoodInput();
static void standby();


//************************************************
//	Custom characters
//************************************************


byte ARROW_UP[] = {
  B00100,
  B01110,
  B10101,
  B00100,
  B00100,
  B00100,
  B00100,
  B00100
};

byte ARROW_DOWN[] = {
  B00100,
  B00100,
  B00100,
  B00100,
  B00100,
  B10101,
  B01110,
  B00100
};

byte ARROW_LEFT[] = {
  B00000,
  B00010,
  B00100,
  B01000,
  B11111,
  B01000,
  B00100,
  B00010
};

byte ARROW_RIGHT[] = {
  B00000,
  B01000,
  B00100,
  B00010,
  B11111,
  B00010,
  B00100,
  B01000
};

enum
{
	CHAR_ARROW_UP,
	CHAR_ARROW_DOWN,
	CHAR_ARROW_LEFT,
	CHAR_ARROW_RIGHT,
};

//************************************************
//	Implementation
//************************************************

// the setup function runs once when you press reset or power the board
void setup() 
{
	//led.SetRed();		delay(1000); led.SetGreen();		delay(1000); led.SetOff();
	//RGB_led.SetRed();	delay(1000); RGB_led.SetGreen();	delay(1000); RGB_led.SetBlue();	delay(1000); RGB_led.SetOff();
	//return;
	SerialMenu::Begin();

	receiver.enableIRIn();
	lcd.init();

	#define CreateLcdChar(id)	lcd.createChar(CHAR_ARROW_##id, ARROW_##id)

	CreateLcdChar(UP);
	CreateLcdChar(DOWN);
	CreateLcdChar(LEFT);
	CreateLcdChar(RIGHT);

	#undef CreateLcdChar

	lcd.clear();
	lcd.noBlink();
	lcd.backlight();

	lcd.setCursor(0, 0);
	lcd.print("* Configurator *");
	lcd.setCursor(0, 1);
	lcd.print("*     Tool     *");

	led.SetRed();		delay(1000); led.SetGreen();		delay(1000); led.SetOff();
	RGB_led.SetRed();	delay(1000); RGB_led.SetGreen();	delay(1000); RGB_led.SetBlue();	delay(1000); RGB_led.SetOff();

	lcd.clear();
	lcd.noBlink();
	lcd.noBacklight();

	standby();
}
//--------------------------------------------------
// the loop function runs over and over again until power down or reset
void loop() 
{
//	return;
//	RGB_led.SetRed();
	StdIR::Key key;

	while (!StdIR::Recv(receiver, key))
	{
		standby_toggler.Toggle();
		delay(10);
	}

	if (key != StdIR::OK)
		return;

	standby_toggler.Stop();

	#define SEND_TO_OWNER(call)	RGB_led.SetGreen();	SerialMenu::Send##call; RGB_led.SetOff()

	SEND_TO_OWNER(RootRequest());

	RGB_led.SetBlue();
	bool ok = response.Receive(2000);
	RGB_led.SetOff();
	
	//LOGGER << "Response is " << ok << NL;

	if (ok)
		run_menu();

	standby();
}
//--------------------------------------------------
static void run_menu()
{
	lcd.clear();
	lcd.noBlink();
	lcd.backlight();

	while (treat_response())
		;

	lcd.clear();
	lcd.noBlink();
	lcd.noBacklight();
}
//--------------------------------------------------
bool receive_answer()
{
	RGB_led.SetBlue();
	bool ok = response.Receive();
	RGB_led.SetOff();

	if (!ok)
	{
		RGB_led.SetRed();
		return false;
	}

	return true;
}
//--------------------------------------------------
static bool treat_response()
{
	//LOGGER << __FUNCTION__ << "  line: " << __LINE__ << NL;

	SerialMenu::Response::Type type = response.GetType();

	switch (type)
	{
		case SerialMenu::Response::Display:
		{
			//LOGGER << __FUNCTION__ << "  line: " << __LINE__ << NL;

			if (!display_text())
			{
				if (key != StdIR::N0 && (key != StdIR::LEFT))
					_LOGGER << __FUNCTION__ << "  line: " << __LINE__ << NL;

				return false;
			}

			break;
		}

		case SerialMenu::Response::SetValue:
		{
			if (!update_value())
			{
				_LOGGER << __FUNCTION__ << "  line: " << __LINE__ << NL;
				return false;
			}

			break;
		}

		case SerialMenu::Response::End	: return false;

		case SerialMenu::Response::Error: 
		{
			lcd.clear();
			lcd.setCursor(0, LCD_TEXT_LINE);
			lcd.print("*** ERROR ***");

			lcd.setCursor(0, LCD_VALUE_LINE);
			lcd.print(response.text_or_value);

			while (!StdIR::Recv(receiver, key))
				delay(10);

			lcd.clear();

			SEND_TO_OWNER(NavigationRequest(response.id, SerialMenu::STAY));

			if (!receive_answer())
			{
				return false;
			}

			break;
		}

		default							:
		{
			_LOGGER << __FUNCTION__ << "  line: " << __LINE__ << NL;
			return false;
		}
	}

	//LOGGER << __FUNCTION__ << "  line: " << __LINE__ << NL;
	return true;
}
//--------------------------------------------------
static bool display_text()
{
	//LOGGER << __FUNCTION__ << "  line: " << __LINE__ << NL;

	lcd.clear();
	lcd.setCursor(0, LCD_TEXT_LINE);

	if (*response.id)
	{
		lcd.print(response.id);
		lcd.print(" ");
	}

	lcd.print(response.text_or_value);

	return navigate();
}
//--------------------------------------------------
static bool navigate()
{
	SerialMenu::NavigationDirection direction;

	uint8_t child_id = *response.children_count ? *response.children_count - '0' : 0;

	if (child_id)
	{
		lcd.setCursor(0, LCD_VALUE_LINE);
		lcd.print("1-");
		lcd.print(response.children_count);
	}

	int idx = 15;
	if (response.up)	{ lcd.setCursor(idx--, LCD_VALUE_LINE); lcd.write(CHAR_ARROW_UP); }
	if (response.down)	{ lcd.setCursor(idx--, LCD_VALUE_LINE); lcd.write(CHAR_ARROW_DOWN); }

	if (!get_navigation_input(direction, child_id))
	{
		if (key != StdIR::N0 && (key != StdIR::LEFT))
			_LOGGER << __FUNCTION__ << "  line: " << __LINE__ << NL;

		return false;
	}

	lcd.setCursor(0, LCD_VALUE_LINE);
	lcd.print("    ");
	lcd.setCursor(14, LCD_VALUE_LINE);
	lcd.print("  ");

	if (child_id)
	{
		SEND_TO_OWNER(GoToChildRequest(response.id, child_id));
	}
	else
	{
		SEND_TO_OWNER(NavigationRequest(response.id, direction));
	}

	if (!receive_answer())
	{
		if (key != StdIR::N0 && (key != StdIR::LEFT))
			_LOGGER << __FUNCTION__ << "  line: " << __LINE__ << NL;

		return false;
	}

	return true;
}
//--------------------------------------------------
static bool get_navigation_input(SerialMenu::NavigationDirection& direction, uint8_t& child_id)
{
	direction			 = NO_DIRECTION;
	uint8_t max_child_id = child_id;
	child_id			 = 0;

	for (;;)
	{
		while (!StdIR::Recv(receiver, key))
			delay(10);

		bool ok = true;

		switch (key)
		{
			#define SET_DIRECTION(id, allow)	case StdIR::id : if(allow) direction = SerialMenu::id;	else ok = false; break

			SET_DIRECTION(UP,		response.up);
			SET_DIRECTION(DOWN,		response.down);
			SET_DIRECTION(RIGHT,	true);
			SET_DIRECTION(LEFT,		true);

			#undef SET_DIRECTION

			case StdIR::OK	: direction = SerialMenu::RIGHT;	break;
			case StdIR::N0	: break;

			#define SET_CHILDID(n)	case StdIR::N##n: if (n > max_child_id) { ok = false; } else { child_id = n; } break
			SET_CHILDID(1);
			SET_CHILDID(2);
			SET_CHILDID(3);
			SET_CHILDID(4);
			SET_CHILDID(5);
			SET_CHILDID(6);
			SET_CHILDID(7);
			SET_CHILDID(8);
			SET_CHILDID(9);

			default: ok = false; break;
		}

		if (ok)
			break;

		led.SetRed();	delay(LED_BLINK_LEN);		led.SetOff();
	}

	led.SetGreen();	delay(LED_BLINK_LEN); led.SetOff();

	return (NO_DIRECTION != direction) || (child_id != 0);
}
//--------------------------------------------------
static bool update_value()
{
	lcd.noBlink();
	lcd.setCursor(0, LCD_VALUE_LINE);
	lcd.print(response.text_or_value);

	bool ok = set_value();

	lcd.noBlink();

	if (ok)
	{
		SEND_TO_OWNER(UpdateRequest(response.id, value.c_str()));
	}
	else
	{
		SEND_TO_OWNER(NavigationRequest(response.id, SerialMenu::LEFT));
	}

	if (!receive_answer())
	{
		return false;
	}

	return true;
}
//--------------------------------------------------
static bool set_value_Latitude();
static bool set_value_Longitude();
static bool set_value_GmtOffset();
static bool set_value_Date();
static bool set_value_Time();
static bool set_value_HourAndMinute();
static bool set_value_OnOff();
static bool set_value_YesNo();
static bool set_value_NoYes();
static bool set_value_Boolean();
static bool set_value_Text();
//--------------------------------------------------
static bool set_value()
{
	switch (response.value_type)
	{
		#define TREAT_CASE(id)	case Menu::Leaf::id : return set_value_##id()

		TREAT_CASE(Latitude);
		TREAT_CASE(Longitude);
		TREAT_CASE(GmtOffset);
		TREAT_CASE(Date);
		TREAT_CASE(Time);
		TREAT_CASE(OnOff);
		TREAT_CASE(Boolean);
		TREAT_CASE(YesNo);
		TREAT_CASE(NoYes);
		TREAT_CASE(HourAndMinute);
		//TREAT_CASE(Text);

		#undef TREAT_CASE
	}


}
//--------------------------------------------------
static bool set_value_Latitude()
{
	return enterSignedReal(Coordinates::Latitude::MIN_VALUE, Coordinates::Latitude::MAX_VALUE, Coordinates::SIGNIFICANT_DECIMAL_DIGITS);
}
//--------------------------------------------------
static bool set_value_Longitude()
{
	return enterSignedReal(Coordinates::Longitude::MIN_VALUE, Coordinates::Longitude::MAX_VALUE, Coordinates::SIGNIFICANT_DECIMAL_DIGITS);
}
//--------------------------------------------------
static bool set_value_GmtOffset()
{
	return enterSignedReal(GmtOffset::MIN_HOURS, GmtOffset::MAX_HOURS, GmtOffset::SIGNIFICANT_DECIMAL_DIGITS, (double)GmtOffset::RESOLUTION_MINUTES / double(MINUTES_PER_HOUR));
}
//--------------------------------------------------
static bool set_value_Date()
{
	char s[DATE_LEN + 1] = { 0 };
	return enterDateTime(s, DATE_LEN);
}
//--------------------------------------------------
static bool set_value_Time()
{
	char s[TIME_LEN + 1] = { 0 };
	return enterDateTime(s, TIME_LEN);
}
//--------------------------------------------------
static bool set_value_HourAndMinute()
{
	char s[HOUR_AND_MINUTE_LEN + 1] = { 0 };
	return enterDateTime(s, HOUR_AND_MINUTE_LEN);
}
//--------------------------------------------------
static bool set_value_OnOff()
{
	return enterBoolean(MENU_TEXT_ON, MENU_TEXT_OFF);
}
//--------------------------------------------------
static bool set_value_Boolean()
{
	return enterBoolean(MENU_TEXT_TRUE, MENU_TEXT_FALSE);
}
//--------------------------------------------------
static bool set_value_YesNo()
{
	return enterBoolean(MENU_TEXT_YES, MENU_TEXT_NO);
}
//--------------------------------------------------
static bool set_value_NoYes()
{
	return enterBoolean(MENU_TEXT_NO, MENU_TEXT_YES);
}
//--------------------------------------------------
static bool set_value_Text()
{
//    enterText()
}
//--------------------------------------------------
static void set_double_string_precision(const String& str, int decimal_digits, double resolution, char* s)
{
	double val = str.toDouble();

	val = dround(val, resolution);
	char* p = s;
	bool add_sign = val >= 0;

	if (add_sign)
	{
		*p = '+';
		p++;
	}

	dtostrf(val, -1, decimal_digits, p);
}
//--------------------------------------------------
static char* get_double_string_precision(const String& str, int decimal_digits, double resolution, char* s)
{
	set_double_string_precision(String(str), decimal_digits, resolution, s);
	return s;
}
//--------------------------------------------------
static char* get_double_string_precision(char* s, int decimal_digits, double resolution)
{
	get_double_string_precision(String(s), decimal_digits, resolution, s);
	return s;
}
//--------------------------------------------------
static void printValue(const char* s)
{
	lcd.setCursor(0, LCD_VALUE_LINE);
	lcd.print(s);
}
//--------------------------------------------------
static bool enterSignedReal(double minval, double maxval, int decimal_digits, double resolution)
{
	lcd.blink();

RESTART:

	value = response.text_or_value;

	char s[MAXLEN + 1] = { 0 };

	set_double_string_precision(value, decimal_digits, resolution, s);

	printValue(s);

	int idx = 0;

	lcd.setCursor(idx, LCD_VALUE_LINE);

	for (;;)
	{
		bool good_input = true;

		StdIR::Key key;

		while (!StdIR::Recv(receiver, key));

		if (StdIR::UP == key)
			goto RESTART;

		if (StdIR::DOWN == key)
			return false;

		if (StdIR::OK == key)
		{
			value = s;
			double val = value.toDouble();

			if (val >= minval && val <= maxval)
				break;
			else
				good_input = false;
		}

		if (idx == 0)
		{
			if (StdIR::STAR == key)
			{
				*s = (*s == '+') ? '-' : '+';
				idx++;
			}
			else
				if (StdIR::RIGHT == key)
				{
					idx++;
				}
				else
				{
					good_input = false;
				}
		}
		else
		{
			switch (key)
			{
				#define NUMERIC_CASE(n) case StdIR::N##n : if(idx <= MAXIDX) { s[idx++] = '0' + n; } else { good_input = false; } break
				NUMERIC_CASE(0);
				NUMERIC_CASE(1);
				NUMERIC_CASE(2);
				NUMERIC_CASE(3);
				NUMERIC_CASE(4);
				NUMERIC_CASE(5);
				NUMERIC_CASE(6);
				NUMERIC_CASE(7);
				NUMERIC_CASE(8);
				NUMERIC_CASE(9);
				#undef NUMERIC_CASE

			case StdIR::LEFT:
			{
				if (idx)
					idx--;
				else
					good_input = false;

				break;
			}

			case StdIR::RIGHT:
			{
				if (s[idx])
					idx++;
				else
					good_input = false;

				break;
			}

			case StdIR::STAR:
			{
				if (idx > 1 && idx < MAXIDX - 1 && (s[idx] = '.' || !strchr(s, '.')))
					s[idx++] = '.';
				else
					good_input = false;

				break;
			}

			case StdIR::DIEZ:
			{

				if (idx > 2)
				{
					lcd.setCursor(idx - 1, LCD_VALUE_LINE);

					if (s[idx--])
					{
						strcpy(s + idx, s + idx + 1);
					}
					else
					{
						s[idx] = 0;
					}
				}
				else
				{
					good_input = false;
				}

				break;
			}

			default:
				good_input = false;
				break;
			}
		}

		if (good_input)
		{
			printValue(s);

			lcd.print(" ");
			lcd.setCursor(idx, LCD_VALUE_LINE);
			onGoodInput();
		}
		else
		{
			onBadInput();
		}
	}

	value = get_double_string_precision(s, decimal_digits, resolution);

	return true;
}
//--------------------------------------------------
static bool enterDateTime(char* s, const int SLEN)
{
	lcd.blink();

RESTART:

	value = response.text_or_value;

	memcpy(s, value.c_str(), SLEN);
	s[SLEN] = 0;

	printValue(s);

	int idx = strlen(s);

	lcd.setCursor(idx, LCD_VALUE_LINE);

	for (;;)
	{
		bool good_input = true;

		StdIR::Key key;

		while (!StdIR::Recv(receiver, key));

		if (StdIR::UP == key)
			goto RESTART;

		if (StdIR::DOWN == key)
			return false;

		if (StdIR::OK == key)
		{
			bool valid;
			
			switch(SLEN)
			{
				case DATE_LEN: valid = Times::IsValidDate(s);	break;
				case TIME_LEN: valid = Times::IsValidTime(s);	break;
				default		 : 
					char temp[TIME_LEN + 1];
					strcpy(temp, s);
					strcat(temp, ":00");
					valid = Times::IsValidTime(temp);
					break;
			}

			if (valid)
				break;
			else
				good_input = false;
		}
		else
		{
			switch (key)
			{
				#define NUMERIC_CASE(n) case StdIR::N##n : if(idx < SLEN) { s[idx++] = '0' + n; } else { good_input = false; } break
				NUMERIC_CASE(0);
				NUMERIC_CASE(1);
				NUMERIC_CASE(2);
				NUMERIC_CASE(3);
				NUMERIC_CASE(4);
				NUMERIC_CASE(5);
				NUMERIC_CASE(6);
				NUMERIC_CASE(7);
				NUMERIC_CASE(8);
				NUMERIC_CASE(9);
				#undef NUMERIC_CASE

			case StdIR::LEFT:
			{
				if (idx)
					idx--;
				else
					good_input = false;

				break;
			}

			case StdIR::RIGHT:
			{
				if (s[idx])
					idx++;
				else
					good_input = false;

				break;
			}

			default:
				good_input = false;
				break;
			}
		}

		if (good_input)
		{
			char c = s[idx];
			if (c && (c < '0' || c > '9'))
			{
				idx += (StdIR::LEFT == key) ? -1 : 1;
			}

			printValue(s);

			lcd.setCursor(idx, LCD_VALUE_LINE);
			onGoodInput();
		}
		else
		{
			onBadInput();
		}
	}

	value = s;

	return true;
}
//--------------------------------------------------
static bool enterBoolean(const char* ON, const char* OFF)	
{
	lcd.noBlink();

RESTART:

	value = response.text_or_value;

	bool val = value == ON;

	for (;;)
	{
		lcd.setCursor(0, LCD_VALUE_LINE);
		lcd.print(val ? ON : OFF); lcd.print(" ");

		bool good_input = true;

		StdIR::Key key;

		while (!StdIR::Recv(receiver, key));

		if (StdIR::OK == key)
		{
			break;
		}

		switch (key)
		{
			case StdIR::UP	: goto RESTART;
			case StdIR::DOWN: return false;

			case StdIR::RIGHT:
			case StdIR::LEFT:
			{
				val = !val;
				break;
			}

			default:
				good_input = false;
				break;
		}

		if (good_input)
			onGoodInput();
		else
			onBadInput();
	}

	value = val ? ON : OFF;

	return true;
}
//--------------------------------------------------
static void onGoodInput()
{
	led.SetGreen(); delay(LED_BLINK_LEN); led.SetOff();
}
//--------------------------------------------------
static void onBadInput()
{
	led.SetRed(); delay(LED_BLINK_LEN); led.SetOff();
}
//--------------------------------------------------
static void standby()
{
	standby_toggler.Start(led.GetGreen(), Toggler::OnTotal(30, 5000));
}
//--------------------------------------------------


