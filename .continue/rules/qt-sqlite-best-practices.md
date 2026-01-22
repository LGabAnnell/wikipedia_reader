---
description: Apply these best practices when working with SQLite databases in Qt
  applications.
alwaysApply: true
---

When using SQLite with Qt:
1. Always use a connection name when adding a database to avoid conflicts
2. Open the database once and reuse the connection rather than opening/closing frequently
3. Use transactions for multiple related operations to improve performance
4. Handle database errors gracefully and emit signals to notify the UI
5. Store the database in QStandardPaths::AppDataLocation for cross-platform compatibility
6. Create indexes for frequently queried columns
7. Use prepared statements with bind values to prevent SQL injection
8. Close and remove the database connection in the destructor for proper cleanup