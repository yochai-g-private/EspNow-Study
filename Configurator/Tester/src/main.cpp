/*
 Name:		TestMenuTool.ino
 Created:	3/31/2020 7:09:29 AM
 Author:	MCP
*/

#include "TimeEx.h"
#include "StdConfig.h"
#include "RTC.h"
#include "Sun.h"
#include "Timer.h"
#include "Kodesh.h"

using namespace NYG;

STD_MAIN_MENU_WITH_RESET();

StdConfig						cfg("TestMenuTool", 0);
StdConfig::SerialMenuContext	ctx(MainMenu, cfg);

Timer timer;

void showSun();
bool never();
void setup()
{
	SerialMenu::Begin();
	RTC::Begin();

	TRACING = true;
	cfg.Load();
	cfg.Show();
	//showSun();

	timer.StartForever(10, SECS);

	LOGGER << "Ready!" << NL;
}

void showSun()
{
	FixTime rise, set;

	if (Sun::GetTodayLocalRiseSetTimes(rise, set))
	{
		LOGGER << "Now    : " << FixTime::Now().GetSeconds() << NL;
		LOGGER << "SunRise: " << rise.ToText() << NL;
		LOGGER << "SunSet : " << set.ToText() << NL;
	}
}

void loop()
{
	SerialMenu::Proceed(ctx);

	if (timer.Test())
	{
	}
}

extern const char*	gbl_build_date = __DATE__;
extern const char*	gbl_build_time = __TIME__;

