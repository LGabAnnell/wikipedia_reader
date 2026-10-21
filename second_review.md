# Source review findings

Review scope: `src/` under the repository's current working tree. The findings below were reproduced with temporary fixture tests; no application source files were changed during the review.

## Findings

### [P2] Gallery uses the global article instead of the visible article

After opening article B from article A and navigating Back to A, the visible article view is A while `ArticleState` still holds B. `ImageHomeModel::loadImagesForCurrentPage()` reads the singleton's page ID, so the gallery displays B's images under A's title. A temporary navigation fixture reproduced this mismatch.

Relevant code: [src/modules/ImageDisplayModule/ImageHomeModel.cpp](src/modules/ImageDisplayModule/ImageHomeModel.cpp#L60)

## Validation

- `./scripts/run_tests.sh`: build succeeded; all 27 registered CTest entries passed.
- A temporary fixture check reproduced the remaining gallery finding before remediation. The check was kept under `/tmp` and was not added to the repository.
- Live Wikipedia API behavior was not tested.
