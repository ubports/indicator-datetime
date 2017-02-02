project (indicator-datetime C CXX)
cmake_minimum_required (VERSION 2.8.9)

list (APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set (PROJECT_VERSION "14.04.0")
set (PACKAGE ${CMAKE_PROJECT_NAME})
set (GETTEXT_PACKAGE "indicator-datetime")
add_definitions (-DGETTEXT_PACKAGE="${GETTEXT_PACKAGE}"
                 -DGNOMELOCALEDIR="${CMAKE_INSTALL_FULL_LOCALEDIR}")

set(ALARM_DEFAULT_SOUND    "/usr/share/sounds/ubuntu/ringtones/Alarm clock.ogg")
set(CALENDAR_DEFAULT_SOUND "/usr/share/sounds/ubuntu/ringtones/Marimbach.ogg")
add_definitions(-DALARM_DEFAULT_SOUND="${ALARM_DEFAULT_SOUND}"
                -DCALENDAR_DEFAULT_SOUND="${CALENDAR_DEFAULT_SOUND}")


##
## Always Be Testing
##
enable_testing()

##
##  GNU standard installation directories
##

include (GNUInstallDirs)
if (EXISTS "/etc/debian_version") # Workaround for libexecdir on debian
  set (CMAKE_INSTALL_LIBEXECDIR "${CMAKE_INSTALL_LIBDIR}")
  set (CMAKE_INSTALL_FULL_LIBEXECDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
endif ()
set (CMAKE_INSTALL_PKGLIBEXECDIR "${CMAKE_INSTALL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")
set (CMAKE_INSTALL_FULL_PKGLIBEXECDIR "${CMAKE_INSTALL_FULL_LIBEXECDIR}/${CMAKE_PROJECT_NAME}")

##
##  Check for prerequisites
##

find_package (PkgConfig REQUIRED)
include (CheckIncludeFile)
include (FindPkgConfig)

pkg_check_modules (SERVICE_DEPS REQUIRED
                   glib-2.0>=2.36
                   gio-unix-2.0>=2.36
                   libical>=0.48
                   libecal-1.2>=3.5
                   libedataserver-1.2>=3.5
                   gstreamer-1.0>=1.2
                   libnotify>=0.7.6
                   url-dispatcher-1>=1
                   properties-cpp>=0.0.1
                   libaccounts-glib>=1.18
                   messaging-menu>=12.10
                   uuid>=2.25
                   ubuntu-app-launch-2)
include_directories (SYSTEM ${SERVICE_DEPS_INCLUDE_DIRS})

##
##  custom targets
##

set (ARCHIVE_NAME ${CMAKE_PROJECT_NAME}-${PROJECT_VERSION})
add_custom_target (dist
                   COMMAND bzr export --root=${ARCHIVE_NAME} ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.gz
                   WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_custom_target (cppcheck COMMAND cppcheck --enable=all -q --error-exitcode=2 --inline-suppr
                   ${CMAKE_SOURCE_DIR}/src
                   ${CMAKE_SOURCE_DIR}/tests)

##
##  Actual building
##

set (CC_WARNING_ARGS " -Wall -Wshadow -Wextra -Wunused -Wformat=2 -Wno-missing-field-initializers")
set (CXX_WARNING_ARGS " -Wall -Wextra -pedantic -Wno-missing-field-initializers")

include_directories (${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories (${CMAKE_CURRENT_BINARY_DIR}/include)

# actually build things
add_subdirectory(include)
add_subdirectory(src)
add_subdirectory(data)
add_subdirectory(po)
add_subdirectory(tests)

# testing & coverage
find_package(CoverageReport)
ENABLE_COVERAGE_REPORT(
  TARGETS indicatordatetimeservice indicator-datetime-service
  TESTS test-actions test-alarm-queue test-clock test-datetime test-exporter test-formatter test-live-actions test-locations test-menu-appointments test-menus test-notification test-notification-response test-planner test-settings test-sound test-timezone-timedated test-utils test-eds-ics-all-day-events test-eds-ics-repeating-events test-eds-ics-nonrepeating-events test-eds-ics-repeating-valarms test-eds-ics-missing-trigger test-eds-ics-tzids test-eds-ics-tzids-2 test-eds-ics-tzids-utc test-eds-ics-non-attending-alarms test-eds-ics-repeating-events-with-individual-change manual-test-snap
  FILTER /usr/include ${CMAKE_BINARY_DIR}/*
)
