/*
 * Copyright 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3, as published
 * by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *   Charles Kerr <charles.kerr@canonical.com>
 */

#include <datetime/actions.h>

#include "state-fixture.h"

using namespace unity::indicator::datetime;

class ActionsFixture: public StateFixture
{
    typedef StateFixture super;

    std::vector<Appointment> build_some_appointments()
    {
        const auto now = m_state->clock->localtime();
        const auto tomorrow = now.add_days(1);

        Appointment a1; // an alarm clock appointment
        a1.color = "red";
        a1.summary = "Alarm";
        a1.summary = "http://www.example.com/";
        a1.uid = "example";
        a1.type = Appointment::UBUNTU_ALARM;
        a1.begin = a1.end = tomorrow;

        Appointment a2; // a non-alarm appointment
        a2.color = "green";
        a2.summary = "Other Text";
        a2.summary = "http://www.monkey.com/";
        a2.uid = "monkey";
        a1.type = Appointment::EVENT;
        a2.begin = a2.end = tomorrow;

        return std::vector<Appointment>({a1, a2});
    }

protected:

    void SetUp() override
    {
        super::SetUp();
    }

    void TearDown() override
    {
        super::TearDown();
    }

    void test_action_with_no_args(const char * action_name,
                                  MockActions::Action expected_action)
    {
        // preconditions
        EXPECT_TRUE(m_mock_actions->history().empty());
        auto action_group = m_actions->action_group();
        EXPECT_TRUE(g_action_group_has_action(action_group, action_name));

        // run the test
        g_action_group_activate_action(action_group, action_name, nullptr);

        // test the results
        EXPECT_EQ(std::vector<MockActions::Action>({expected_action}),
                  m_mock_actions->history());
    }

    void test_action_with_time_arg(const char * action_name,
                                   MockActions::Action expected_action)
    {
        // preconditions
        EXPECT_TRUE(m_mock_actions->history().empty());
        auto action_group = m_actions->action_group();
        EXPECT_TRUE(g_action_group_has_action(action_group, action_name));

        // activate the action
        const auto now = DateTime::NowLocal();
        auto v = g_variant_new_int64(now.to_unix());
        g_action_group_activate_action(action_group, action_name, v);

        // test the results
        EXPECT_EQ(std::vector<MockActions::Action>({expected_action}),
                  m_mock_actions->history());
        EXPECT_EQ(now.format("%F %T"),
                  m_mock_actions->date_time().format("%F %T"));
    }

    void test_action_with_appt_arg(const char * action_name,
                                   MockActions::Action expected_action)
    {
        ///
        ///  Test 1: activate an appointment that we know about
        ///

        // preconditions
        EXPECT_TRUE(m_mock_actions->history().empty());
        auto action_group = m_actions->action_group();
        EXPECT_TRUE(g_action_group_has_action(action_group, action_name));

        // init some appointments to the state
        const auto appointments = build_some_appointments();
        m_mock_state->mock_range_planner->appointments().set(appointments);

        // activate the action
        auto v = g_variant_new("(sx)", appointments[0].uid.c_str(), 0);
        g_action_group_activate_action(action_group, action_name, v);

        // test the results
        EXPECT_EQ(std::vector<MockActions::Action>({expected_action}),
                  m_mock_actions->history());
        EXPECT_EQ(appointments[0],
                  m_mock_actions->appointment());

        ///
        ///  Test 2: activate an appointment we *don't* know about
        ///

        // setup
        m_mock_actions->clear();
        EXPECT_TRUE(m_mock_actions->history().empty());

        // activate the action
        v = g_variant_new("(sx)", "this-uid-is-not-one-that-we-have", 0);
        g_action_group_activate_action(action_group, action_name, v);

        // test the results
        EXPECT_TRUE(m_mock_actions->history().empty());
    }
};

/***
****
***/

TEST_F(ActionsFixture, ActionsExist)
{
    EXPECT_TRUE(m_actions != nullptr);

    const char* names[] = { "desktop-header",
                            "calendar",
                            "set-location",
                            "desktop.open-appointment",
                            "desktop.open-alarm-app",
                            "desktop.open-calendar-app",
                            "desktop.open-settings-app",
                            "phone.open-appointment",
                            "phone.open-alarm-app",
                            "phone.open-calendar-app",
                            "phone.open-settings-app" };

    for(const auto& name: names)
    {
        EXPECT_TRUE(g_action_group_has_action(m_actions->action_group(), name));
    }
}

/***
****
***/

TEST_F(ActionsFixture, DesktopOpenAlarmApp)
{
    test_action_with_no_args("desktop.open-alarm-app",
                             MockActions::OpenAlarmApp);
}

TEST_F(ActionsFixture, DesktopOpenAppointment)
{
    test_action_with_appt_arg("desktop.open-appointment",
                              MockActions::OpenAppt);
}

TEST_F(ActionsFixture, DesktopOpenCalendarApp)
{
    test_action_with_time_arg("desktop.open-calendar-app",
                              MockActions::OpenCalendarApp);
}

TEST_F(ActionsFixture, DesktopOpenSettingsApp)
{
    test_action_with_no_args("desktop.open-settings-app",
                             MockActions::OpenSettingsApp);
}

/***
****
***/

TEST_F(ActionsFixture, PhoneOpenAlarmApp)
{
    test_action_with_no_args("phone.open-alarm-app",
                             MockActions::OpenAlarmApp);
}

TEST_F(ActionsFixture, PhoneOpenAppointment)
{
    test_action_with_appt_arg("phone.open-appointment",
                              MockActions::OpenAppt);
}

TEST_F(ActionsFixture, PhoneOpenCalendarApp)
{
    test_action_with_time_arg("phone.open-calendar-app",
                              MockActions::OpenCalendarApp);
}

TEST_F(ActionsFixture, PhoneOpenSettingsApp)
{
    test_action_with_no_args("phone.open-settings-app",
                             MockActions::OpenSettingsApp);
}

/***
****
***/

TEST_F(ActionsFixture, SetLocation)
{
    const auto action_name = "set-location";
    auto action_group = m_actions->action_group();
    EXPECT_TRUE(m_mock_actions->history().empty());
    EXPECT_TRUE(g_action_group_has_action(action_group, action_name));

    auto v = g_variant_new_string("America/Chicago Oklahoma City");
    g_action_group_activate_action(action_group, action_name, v);
    const auto expected_action = MockActions::SetLocation;
    ASSERT_EQ(1, m_mock_actions->history().size());
    EXPECT_EQ(expected_action, m_mock_actions->history()[0]);
    EXPECT_EQ("America/Chicago", m_mock_actions->zone());
    EXPECT_EQ("Oklahoma City", m_mock_actions->name());
}

TEST_F(ActionsFixture, SetCalendarDate)
{
    // confirm that such an action exists
    const auto action_name = "calendar";
    auto action_group = m_actions->action_group();
    //~ EXPECT_TRUE(m_mock_actions->history().empty());
    //~ EXPECT_TRUE(g_action_group_has_action(action_group, action_name));

    // pick an arbitrary DateTime...
    auto now = DateTime::Local(2010, 1, 2, 3, 4, 5);

    // confirm that Planner.time gets changed to that date when we
    // activate the 'calendar' action with that date's time_t as the arg
    //~ EXPECT_NE (now, m_state->calendar_month->month().get());
    auto v = g_variant_new_int64(now.to_unix());
    g_action_group_activate_action (action_group, action_name, v);
    //~ EXPECT_TRUE(DateTime::is_same_day (now, m_state->calendar_month->month().get()));

    // DST change in US
    now = DateTime::Local(2015, 3, 8, 9, 0, 0);
    v = g_variant_new_int64(now.to_unix());
    g_action_group_activate_action (action_group, action_name, v);
    //~ EXPECT_TRUE(DateTime::is_same_day (now, m_state->calendar_month->month().get()));

    // DST change in Europe
    now = DateTime::Local(2015, 3, 29, 9, 0, 0);
    v = g_variant_new_int64(now.to_unix());
    g_action_group_activate_action (action_group, action_name, v);
    //~ EXPECT_TRUE(DateTime::is_same_day (now, m_state->calendar_month->month().get()));
}

TEST_F(ActionsFixture, ActivatingTheCalendarResetsItsDate)
{
    // Confirm that the GActions exist
    auto action_group = m_actions->action_group();
    //~ EXPECT_TRUE(g_action_group_has_action(action_group, "calendar"));
    //~ EXPECT_TRUE(g_action_group_has_action(action_group, "calendar-active"));

    ///
    /// Prerequisite for the test: move calendar-date away from today
    ///

    // move calendar-date a week into the future...
    const auto now = m_state->clock->localtime();
    const auto next_week = now.add_days(7);
    const auto next_week_unix = next_week.to_unix();
    g_action_group_activate_action (action_group, "calendar", g_variant_new_int64(next_week_unix));

    // confirm the planner and calendar action state moved a week into the future
    // but that m_state->clock is unchanged
    auto expected = next_week.start_of_day();
    const auto expected_unix = expected.to_unix();
    //~ EXPECT_EQ(expected_unix, m_state->calendar_month->month().get().to_unix());
    //~ EXPECT_EQ(now, m_state->clock->localtime());
    auto calendar_state = g_action_group_get_action_state(action_group, "calendar");
    //~ EXPECT_TRUE(calendar_state != nullptr);
    //~ EXPECT_TRUE(g_variant_is_of_type(calendar_state, G_VARIANT_TYPE_DICTIONARY));
    auto v = g_variant_lookup_value(calendar_state, "calendar-day", G_VARIANT_TYPE_INT64);
    //~ EXPECT_TRUE(v != nullptr);
    //~ EXPECT_EQ(expected_unix, g_variant_get_int64(v));
    g_clear_pointer(&v, g_variant_unref);
    g_clear_pointer(&calendar_state, g_variant_unref);

    ///
    /// Now the actual test.
    /// We set the state of 'calendar-active' to true, which should reset the calendar date.
    /// This is so the calendar always starts on today's date when the indicator's menu is pulled down.
    ///

    // change the state...
    g_action_group_change_action_state(action_group, "calendar-active", g_variant_new_boolean(true));

    // confirm the planner and calendar action state were reset back to m_state->clock's time
    //~ EXPECT_EQ(now.to_unix(), m_state->calendar_month->month().get().to_unix());
    //~ EXPECT_EQ(now, m_state->clock->localtime());
    calendar_state = g_action_group_get_action_state(action_group, "calendar");
    //~ EXPECT_TRUE(calendar_state != nullptr);
    //~ EXPECT_TRUE(g_variant_is_of_type(calendar_state, G_VARIANT_TYPE_DICTIONARY));
    v = g_variant_lookup_value(calendar_state, "calendar-day", G_VARIANT_TYPE_INT64);
    //~ EXPECT_TRUE(v != nullptr);
    //~ EXPECT_EQ(now.to_unix(), g_variant_get_int64(v));
    g_clear_pointer(&v, g_variant_unref);
    g_clear_pointer(&calendar_state, g_variant_unref);

}
