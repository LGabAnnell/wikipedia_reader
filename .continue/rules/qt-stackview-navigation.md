---
description: This rule applies to all Qt Quick applications using StackView for
  navigation. It ensures a consistent and intuitive navigation experience for
  users.
alwaysApply: true
---

When implementing navigation in Qt Quick using StackView, always ensure that views are pushed onto the stack without unnecessarily popping existing views. This preserves the navigation history and allows users to navigate back using the back button. Use `StackView.push()` to add new views and `StackView.pop()` to remove them.