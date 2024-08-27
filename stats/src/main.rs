mod entities;

use anyhow::Result;
use chrono::{Local, NaiveDate, TimeDelta, TimeZone};
use entities::prelude::{UserInfo, UserStatus};
use entities::user_info::Model as UserInfoModel;
use entities::user_status::Model as UserStatusModel;
use humantime::format_duration;
use log::{info, warn};
use sea_orm::{ColumnTrait, Database, DatabaseConnection, EntityTrait, QueryFilter};
use sea_orm::{ConnectOptions, QueryOrder};
use std::collections::HashMap;
use std::time::Duration;

const DATABASE_URL: &str = "sqlite:./../tg_eye_stats.sqlite3?mode=ro";

fn online_str(is_online: bool) -> &'static str {
    if is_online {
        "online"
    } else {
        "offline"
    }
}

fn online_time_per_day_in_seconds(s: &Vec<UserStatusModel>) -> HashMap<NaiveDate, u64> {
    let mut num_sec_per_day: HashMap<NaiveDate, u64> = HashMap::new();
    let mut last_status: Option<&UserStatusModel> = None;

    for status in s {
        let current_datetime = Local.timestamp_opt(status.timestamp as i64, 0).unwrap();
        let current_day = current_datetime.date_naive();
        let current_online = status.status == 1;

        if let Some(last) = last_status {
            let last_datetime = Local.timestamp_opt(last.timestamp as i64, 0).unwrap();
            let last_day = last_datetime.date_naive();
            let last_online = last.status == 1;

            let duration =
                Duration::from_secs((current_datetime - last_datetime).num_seconds() as u64);

            warn!(
                "{} {}->{} to {}",
                last_datetime.format("%-d %B %H:%M:%S"),
                online_str(last_online),
                online_str(current_online),
                format_duration(duration)
            );
            if current_online == last_online {
                // if duration_with_same_status > Duration::from_secs(60 * 6) {
                //     warn!(
                //         "online = {} duration = {} id: {}",
                //         last_online,
                //         format_duration(duration_with_same_status),
                //         status.telegram_user_id
                //     );
            }
            if last_online && !current_online {
                // Transition from online to offline
                if current_day == last_day {
                    // Same day
                    *num_sec_per_day.entry(current_day).or_insert(0) += duration.as_secs();
                } else {
                    // Span multiple days
                    let mut day = last_day;
                    while day <= current_day {
                        let start = if day == last_day {
                            last_datetime
                        } else {
                            Local
                                .from_local_date(&day)
                                .unwrap()
                                .and_hms_opt(0, 0, 0)
                                .unwrap()
                        };

                        let end = if day == current_day {
                            current_datetime
                        } else {
                            Local
                                .from_local_date(&day)
                                .unwrap()
                                .and_hms_opt(23, 59, 59)
                                .unwrap()
                        };

                        let duration = (end - start).num_seconds();
                        *num_sec_per_day.entry(day).or_insert(0) += duration as u64;

                        day += TimeDelta::days(1);
                    }
                }
            }
        }

        last_status = Some(status);
    }

    num_sec_per_day
}

fn calculate_median_time_per_day(times: &Vec<(NaiveDate, u64)>) -> Option<u64> {
    if times.is_empty() {
        return None;
    }

    // Extract just the times (u64 values) into a separate vector
    let mut durations: Vec<u64> = times.iter().map(|(_, duration)| *duration).collect();

    // Sort the durations
    durations.sort_unstable();

    let len = durations.len();
    if len % 2 == 0 {
        // If even number of elements, average the two middle values
        let mid = len / 2;
        Some((durations[mid - 1] + durations[mid]) / 2)
    } else {
        // If odd number of elements, return the middle value
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

        let mut online_times = online_time_per_day_in_seconds(&statuses)
            .into_iter()
            .collect::<Vec<(NaiveDate, u64)>>();
        online_times.sort_by(|info1, info2| info1.0.partial_cmp(&info2.0).unwrap());
        let median_time_per_day =
            Duration::from_secs(calculate_median_time_per_day(&online_times).unwrap_or_default());

        info!(
            "{0}\tid:\t{1}\t{2}\tmedian online time per day: {3}",
            user.full_name.as_ref().unwrap_or(default_name),
            user.telegram_user_id,
            last_online,
            format_duration(median_time_per_day)
        );

        // for info in online_times {
        //     info!(
        //         "Online @{} is {}",
        //         info.0.format("%-d %B"),
        //         format_duration(Duration::from_secs(info.1))
        //     )
        // }
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
