---
alwaysApply: true
---

Always ensure that event filters in Qt are non-blocking for events they do not handle. Only intercept and handle the specific events you need (e.g., Qt::BackButton), and return false for all others to allow propagation.