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

#include <datetime/dbus-shared.h>
#include <datetime/actions-live.h>

#include <url-dispatcher.h>

#include <glib.h>

#include <sstream>

namespace unity {
namespace indicator {
namespace datetime {

/***
****
***/

LiveActions::LiveActions(const std::shared_ptr<State>& state_in):
    Actions(state_in)
{
}

void LiveActions::execute_command(const std::string& cmdstr)
{
    const auto cmd = cmdstr.c_str();
    g_debug("Issuing command '%s'", cmd);

    GError* error = nullptr;
    if (!g_spawn_command_line_async(cmd, &error))
    {
        g_warning("Unable to start \"%s\": %s", cmd, error->message);
        g_error_free(error);
    }
}

void LiveActions::dispatch_url(const std::string& url)
{
    g_debug("Dispatching url '%s'", url.c_str());
    url_dispatch_send(url.c_str(), nullptr, nullptr);
}

/***
****
***/

LiveActions::Desktop LiveActions::get_desktop()
{
    static bool cached = false;
    static LiveActions::Desktop result = LiveActions::OTHER;

    if (cached) {
        return result;
    }

    // check for unity8
    if (g_getenv ("MIR_SOCKET") != nullptr) {
        result = LiveActions::UNITY8;
     } else {
        const gchar *xdg_current_desktop = g_getenv ("XDG_CURRENT_DESKTOP");
        if (xdg_current_desktop != NULL) {
            gchar **desktop_names = g_strsplit (xdg_current_desktop, ":", 0);
            for (size_t i = 0; desktop_names[i]; ++i) {
                if (!g_strcmp0 (desktop_names[i], "Unity")) {
                    result = LiveActions::UNITY7;
                    break;
                }
            }
            g_strfreev (desktop_names);
        }
    }
    cached = true;
    return result;
}

void LiveActions::open_alarm_app()
{
    switch(get_desktop()) {
        case LiveActions::UNITY8:
            dispatch_url("appid://com.ubuntu.clock/clock/current-user-version");
            break;
        case LiveActions::UNITY7:
        default:
            execute_command("evolution -c calendar");
    }
}

void LiveActions::open_appointment(const Appointment& appt, const DateTime& date)
{
    switch(get_desktop()) {
        case LiveActions::UNITY8:
            unity8_open_appointment(appt, date);
            break;
        case LiveActions::UNITY7:
        default:
            open_calendar_app(date);
    }
}

void LiveActions::open_calendar_app(const DateTime& dt)
{
    switch(get_desktop()) {
        case LiveActions::UNITY8:
        {
            const auto utc = dt.to_timezone("UTC");
            auto cmd = utc.format("calendar://startdate=%Y-%m-%dT%H:%M:%S+00:00");
            dispatch_url(cmd);
            break;
        }
        case LiveActions::UNITY7:
        default:
        {
            const auto utc = dt.start_of_day().to_timezone("UTC");
            auto cmd = utc.format("evolution \"calendar:///?startdate=%Y%m%dT%H%M%SZ\"");
            execute_command(cmd.c_str());
        }
    }
}

void LiveActions::open_settings_app()
{
    switch(get_desktop()) {
    case LiveActions::UNITY8:
        dispatch_url("settings:///system/time-date");
        break;
    case LiveActions::UNITY7:
        execute_command("unity-control-center datetime");
        break;
    default:
        execute_command("gnome-control-center datetime");
    }
}


bool LiveActions::desktop_has_calendar_app() const
{
    static bool inited = false;
    static bool have_calendar = false;

    if (G_UNLIKELY(!inited))
    {
        inited = true;

#if 0
        auto all = g_app_info_get_all_for_type ("text/calendar");
        for(auto l=all; !have_calendar && l!=nullptr; l=l->next)
        {
            auto app_info = static_cast<GAppInfo*>(l->data);

            if (!g_strcmp0("evolution.desktop", g_app_info_get_id(app_info)))
                have_calendar = true;
        }

        g_list_free_full(all, (GDestroyNotify)g_object_unref);
#else
        /* Work around http://pad.lv/1296233 for Trusty...
           let's revert this when the GIO bug is fixed. */
        auto executable = g_find_program_in_path("evolution");
        have_calendar = executable != nullptr;
        g_free(executable);
#endif
    }

    return have_calendar;
}

void LiveActions::unity8_open_appointment(const Appointment& appt, const DateTime& date)
{
    if (!appt.activation_url.empty())
    {
        dispatch_url(appt.activation_url);
    }
    else switch (appt.type)
    {
        case Appointment::UBUNTU_ALARM:
            open_alarm_app();
            break;

        case Appointment::EVENT:
        default:
            open_calendar_app(date);
            break;
    }
}

/***
****
***/

namespace
{

struct setlocation_data
{
  std::string tzid;
  std::string name;
  std::shared_ptr<Settings> settings;
};

static void
on_datetime1_set_timezone_response(GObject       * object,
                                   GAsyncResult  * res,
                                   gpointer        gdata)
{
  GError* err = nullptr;
  auto response = g_dbus_proxy_call_finish(G_DBUS_PROXY(object), res, &err);
  auto data = static_cast<struct setlocation_data*>(gdata);

  if (err != nullptr)
    {
      if (!g_error_matches(err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning("Could not set new timezone: %s", err->message);

      g_error_free(err);
    }
  else
    {
      data->settings->timezone_name.set(data->tzid + " " + data->name);
      g_variant_unref(response);
    }

  delete data;
}

static void
on_datetime1_proxy_ready (GObject      * object G_GNUC_UNUSED,
                          GAsyncResult * res,
                          gpointer       gdata)
{
  auto data = static_cast<struct setlocation_data*>(gdata);

  GError * err = nullptr;
  auto proxy = g_dbus_proxy_new_for_bus_finish(res, &err);
  if (err != nullptr)
    {
      if (!g_error_matches(err, G_IO_ERROR, G_IO_ERROR_CANCELLED))
        g_warning("Could not grab DBus proxy for timedated: %s", err->message);

      g_error_free(err);

      delete data;
    }
  else
    {
      g_dbus_proxy_call(proxy,
                        Bus::Timedate1::Methods::SET_TIMEZONE,
                        g_variant_new ("(sb)", data->tzid.c_str(), TRUE),
                        G_DBUS_CALL_FLAGS_NONE,
                        -1,
                        nullptr,
                        on_datetime1_set_timezone_response,
                        data);

      g_object_unref (proxy);
    }
}

} // unnamed namespace


void LiveActions::set_location(const std::string& tzid, const std::string& name)
{
    g_return_if_fail(!tzid.empty());
    g_return_if_fail(!name.empty());

    auto data = new struct setlocation_data;
    data->tzid = tzid;
    data->name = name;
    data->settings = state()->settings;

    g_dbus_proxy_new_for_bus (G_BUS_TYPE_SYSTEM,
                              G_DBUS_PROXY_FLAGS_NONE,
                              nullptr,
                              Bus::Timedate1::BUSNAME,
                              Bus::Timedate1::ADDR,
                              Bus::Timedate1::IFACE,
                              nullptr,
                              on_datetime1_proxy_ready,
                              data);
}

/***
****
***/

} // namespace datetime
} // namespace indicator
} // namespace unity
