# Code change workflow

Make changes on your own branch, then open a pull request so the code and test results can be reviewed together:

```powershell
git switch main
git pull --ff-only
git switch -c feat/change-name
git status
```

After editing, run the core and gateway tests and build the firmware according to `docs/TESTING.md`. Stage the files that changed, review the diff, then commit and push:

```powershell
git add -A
git diff --cached
git commit -m "Describe the concrete change"
git push -u origin feat/change-name
```

Describe the problem, behavior changes, and test results in the pull request. Distinguish software tests from physical ESP32 tests. The protected `main` branch requires a pull request, up-to-date passing results from `Native C++ tests` and `ESP32 firmware build`, and resolved review conversations before merging. No external reviewer approval is required. Force pushes and branch deletion are blocked, including for the repository owner.

`Secrets.h`, `.pio`, `.venv`, build outputs, and local logs are ignored by Git. Keep network credentials only in local configuration. Avoid editing `legacy/` when fixing the active firmware; the archive is a reference for comparison with previous versions.
