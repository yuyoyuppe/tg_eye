mod entities;

use anyhow::Result;
use chrono::{DateTime, Local, NaiveDateTime, NaiveTime, TimeDelta, TimeZone};
use core::ops::Range;
use entities::prelude::{UserInfo, UserStatus};
use entities::user_info::Model as UserInfoModel;
use entities::user_status::Model as UserStatusModel;
use humantime::format_duration;
use iset::IntervalSet;
use itertools::Itertools;
use log::info;
use sea_orm::{ColumnTrait, Database, DatabaseConnection, EntityTrait, QueryFilter};
use sea_orm::{ConnectOptions, QueryOrder};
use std::collections::HashMap;
use std::time::Duration;

const DATABASE_URL: &str = "sqlite:./../tg_eye_stats.sqlite3?mode=ro";

fn build_online_intervals(s: &[UserStatusModel]) -> IntervalSet<DateTime<Local>> {
    let mut intervals: IntervalSet<DateTime<Local>> = IntervalSet::new();
    _ = s
        .iter()
        .map(|s| {
            let is_online = s.status == 1;
            let datetime = Local.timestamp_opt(s.timestamp as i64, 0).unwrap();
            (is_online, datetime)
        })
        .tuple_windows()
        .fold(
            None,
            |start_online, ((cur_online, cur_dt), (next_online, next_dt))| match (
                cur_online,
                next_online,
            ) {
                (true, true) => start_online.or(Some(cur_dt)),
                (true, false) => {
                    let start = start_online.unwrap_or(cur_dt);
                    if start != next_dt {
                        intervals.insert(start..next_dt);
                    }
                    None
                }
                (false, true) => {
                    assert_eq!(start_online, None);
                    Some(next_dt)
                }
                (false, false) => {
                    assert_eq!(start_online, None);
                    None
                }
            },
        );

    intervals
}

fn interval_duration(interval: Range<DateTime<Local>>) -> Duration {
    Duration::from_secs((interval.end - interval.start).num_seconds() as u64)
}

fn clip_interval(
    interval: Range<DateTime<Local>>,
    start: DateTime<Local>,
    end: DateTime<Local>,
) -> Range<DateTime<Local>> {
    Range::<DateTime<Local>> {
        start: interval.start.max(start),
        end: interval.end.min(end),
    }
}

fn online_time_per_day(
    intervals: &IntervalSet<DateTime<Local>>,
) -> HashMap<DateTime<Local>, Duration> {
    let mut result = HashMap::new();
    let range = if let Some(r) = intervals.range() {
        r
    } else {
        return result;
    };
    let first_day_start: DateTime<Local> = range
        .start
        .date_naive()
        .and_time(NaiveTime::MIN)
        .and_utc()
        .into();
    let last_day_end: DateTime<Local> = range
        .end
        .date_naive()
        .and_time(NaiveDateTime::MAX.time())
        .and_utc()
        .into();

    let mut day_start = first_day_start;
    while day_start <= last_day_end {
        let day_end = (day_start + TimeDelta::days(1))
            .with_time(NaiveTime::MIN)
            .unwrap();

        let daily_duration = intervals
            .iter(day_start..day_end)
            .map(|i| {
                let clipped = clip_interval(i, day_start, day_end);
                interval_duration(clipped)
            })
            .sum();

        if daily_duration > Duration::from_secs(1) {
            result.insert(day_start, daily_duration);
        }
        day_start = day_end;
    }

    result
}

fn format_datetime(dt: DateTime<Local>) -> String {
    dt.format("%Y-%m-%d %H:%M:%S").to_string()
}

fn print_long_offline_intervals(intervals: &IntervalSet<DateTime<Local>>) {
    if let Some(range) = intervals.range() {
        let mut last_end = range.start;
        let three_hours = TimeDelta::hours(3);

        for interval in intervals.iter(range.start..range.end) {
            if interval.start > last_end {
                let offline_duration = interval.start - last_end;
                if offline_duration > three_hours {
                    println!(
                        "Offline interval: {} to {} (duration: {})",
                        format_datetime(last_end),
                        format_datetime(interval.start),
                        format_duration(Duration::from_secs(offline_duration.num_seconds() as u64))
                    );
                }
            }
            last_end = last_end.max(interval.end);
        }

        // Check for offline interval after the last online interval
        if range.end > last_end {
            let offline_duration = range.end - last_end;
            if offline_duration > three_hours {
                println!(
                    "Offline interval: {} to {} (duration: {})",
                    format_datetime(last_end),
                    format_datetime(range.end),
                    format_duration(Duration::from_secs(offline_duration.num_seconds() as u64))
                );
            }
        }
    }
}

fn calculate_median_time_per_day(times: &[(DateTime<Local>, Duration)]) -> Option<Duration> {
    if times.is_empty() {
        return None;
    }

    let mut durations: Vec<Duration> = times.iter().map(|(_, duration)| *duration).collect();

    durations.sort_unstable();

    let len = durations.len();
    if len % 2 == 0 {
        let mid = len / 2;
        Some((durations[mid - 1] + durations[mid]) / 2)
    } else {
        Some(durations[len / 2])
    }
}

async fn print_users(users: &Vec<UserInfoModel>, db: &DatabaseConnection) -> Result<()> {
    let default_name = &String::from("<no_name>");
    for user in users {
        let statuses = UserStatus::find()
            .filter(entities::user_status::Column::TelegramUserId.eq(user.telegram_user_id))
            .order_by_asc(entities::user_status::Column::Timestamp)
            .all(db)
            .await?;

        let last_online = match statuses.iter().rev().find(|s| s.status == 1) {
            Some(s) => format!("Last online was {}", format_timestamp_to_ago(s.timestamp)),
            None => String::from("Last online: ---"),
        };

        let intervals = build_online_intervals(&statuses);

        let mut online_times = online_time_per_day(&intervals)
            .into_iter()
            .collect::<Vec<(_, _)>>();
        online_times.sort_by(|info1, info2| info1.0.partial_cmp(&info2.0).unwrap());
        let median_time_per_day = calculate_median_time_per_day(&online_times).unwrap_or_default();

        info!(
            "{0}\tid:\t{1}\t{2}\tmedian online time per day: {3}",
            user.full_name.as_ref().unwrap_or(default_name),
            user.telegram_user_id,
            last_online,
            format_duration(median_time_per_day)
        );

        if cfg!(debug_assertions) {
            print_long_offline_intervals(&intervals);
            for info in online_times {
                info!("{}: {}", info.0.format("%-d %B"), format_duration(info.1))
            }
        }
    }

    Ok(())
}

fn format_timestamp_to_ago(timestamp: i32) -> String {
    let datetime = Local.timestamp_opt(timestamp as i64, 0).unwrap();
    let duration = Local::now().signed_duration_since(datetime);
    let std_duration = Duration::from_secs(duration.num_seconds().unsigned_abs());
    format_duration(std_duration).to_string()
}

async fn run() -> Result<()> {
    let mut opt = ConnectOptions::new(DATABASE_URL.to_owned());
    opt.sqlx_logging(false);
    let db = &Database::connect(opt).await?;
    let users = UserInfo::find().all(db).await?;
    // let statuses = UserStatus::find().all(db).await?;
    print_users(&users, db).await?;
    Ok(())
}

fn main() -> Result<()> {
    let rt = tokio::runtime::Builder::new_multi_thread()
        .enable_all()
        .build()?;
    colog::init();

    rt.block_on(async { run().await })?;
    Ok(())
}
