# Task Scheduler

A C++ command-line task management application featuring automatic notifications and SQLite-based persistent storage.

## Features

- **Task Management**

  - Create, update, and delete tasks
  - Set due dates and reminder times
  - Mark tasks as completed
  - List all or pending tasks

- **Automatic Notifications**

  - Console notifications with color formatting
  - Background checker that runs every 15 seconds
  - Email notifications (currently being created)

- **Flexible Time Input**
  - Absolute dates: `YYYY-MM-DD HH:MM`
  - Relative times: `+minutes`

## Requirements

- Windows OS
- MinGW G++ compiler with C++23 support
- SQLite3
- libcurl (for upcoming email notifications)

## Installation

1. Install dependencies:

   ```powershell
   # Install MinGW-w64 with C++23 support
   # Install SQLite3
   # Install libcurl and place in C:/curl/curl-8.13.0_1-win64-mingw/
   ```

2. Clone and build:
   ```powershell
   git clone https://github.com/yourusername/TaskScheduler.git
   cd TaskScheduler
   mingw32-make all
   ```

## Usage

### Starting the Application

```powershell
.\task_scheduler.exe --cli
```

### Available Commands

- `help` - Display available commands
- `add <description> <due_date> <reminder_minutes>` - Create new task
- `list [pending|all]` - List tasks
- `update <id> <description> <due_date> <reminder_minutes>` - Update task
- `delete <id>` - Delete task
- `complete <id>` - Mark task as completed
- `schedule <id> [console|email]` - Schedule task notifications
- `check` - Manual check for due notifications
- `email <recipient> <smtp_server> <port>` - Configure email settings (coming soon)
- `exit` or `quit` - Exit application

### Examples

```powershell
# Add a task due in 30 minutes
add "Review code" "+30" 5

# Add a task for specific date/time
add "Team meeting" "2025-04-10 15:00" 15

# Schedule console notifications
schedule 1 console

# List pending tasks
list pending
```

## Notification System

### Console Notifications

- Color-coded output for better visibility
- Sound alerts for important reminders
- Automatic background checking every 15 seconds
- Detailed task information display

### Email Notifications

Email notification functionality is currently being created. When completed, it will support:

- SMTP server configuration
- TLS/SSL security
- Custom email templates
- Gmail compatibility

## Project Structure

```
TaskScheduler/
├── include/           # Header files
│   ├── core/         # Core functionality
│   ├── database/     # Database handling
│   └── notifications/# Notification system
├── src/              # Implementation files
├── Makefile         # Build configuration
└── README.md        # This file
```

## Building from Source

```powershell
# Standard build
mingw32-make all

# Clean build
mingw32-make clean all

# Debug build
mingw32-make debug
```

## License

This project is licensed under the MIT License.
