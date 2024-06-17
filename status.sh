#!/bin/sh
# Report database size
db_size=$(du -h tg_eye_stats.sqlite3 | cut -f1)
echo "Database size is $db_size"

# Report the latest write to the database
printf "Last status update was written "
timestamp=$(sqlite3 -readonly tg_eye_stats.sqlite3 "SELECT timestamp FROM user_status ORDER BY timestamp DESC LIMIT 1;")
datediff "$(date -d @$timestamp '+%Y-%m-%d %H:%M:%S')" "$(date '+%Y-%m-%d %H:%M:%S')" -f '%d days %H hours %M minutes %S seconds ago'

# Report process uptime
APP_PID=$(pgrep -f /tg_eye)
if [ -z "$APP_PID" ]; then
  echo "tg_eye is not running!"
  exit 1
fi
START_TIME=$(ps -p $APP_PID -o lstart=)
START_TIMESTAMP=$(date -d "$START_TIME" +%s)
CURRENT_TIMESTAMP=$(date +%s)
UPTIME_SECONDS=$((CURRENT_TIMESTAMP - START_TIMESTAMP))
UPTIME=$(datediff "$(date -d @$START_TIMESTAMP '+%Y-%m-%d %H:%M:%S')" "$(date '+%Y-%m-%d %H:%M:%S')" -f '%d days %H hours %M minutes %S seconds')
echo "tg_eye has been running for $UPTIME"