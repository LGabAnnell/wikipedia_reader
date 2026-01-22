---
globs: '["**/*.qml","**/NavigationState.*"]'
description: This rule ensures proper stack-based navigation in Qt applications
  using StackView, which provides a more predictable and consistent user
  experience for back/forward navigation.
alwaysApply: false
---

Always use push/pop operations for StackView navigation instead of replace operations. When implementing a 'Home' button, pop all items from the stack to return to the root view rather than pushing a new instance of the root view. Ensure that navigation actions are consistent across all components.