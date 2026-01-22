---
description: Apply these error handling practices for robust database operations
  in Qt applications.
alwaysApply: true
---

When implementing database operations in Qt:
1. Always check if the database is open before executing queries
2. Check if queries are prepared successfully before execution
3. Use executeQuery helper methods to handle errors consistently
4. Emit databaseError signals with descriptive messages for UI feedback
5. Rollback transactions on failure before emitting historyChanged signals
6. Include the actual SQL query in error messages for debugging
7. Use tr() for user-facing error messages to support localization