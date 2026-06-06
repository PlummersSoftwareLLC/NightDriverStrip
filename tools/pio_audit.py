import configparser
import os
import subprocess
import sys

try:
    Import("env")
except Exception:
    env = None

# Audit scripts to run (located in tools/).
AUDIT_SCRIPTS = [
    "audit_globals_order.py",
    "audit_include_rules.py"
]

# PlatformIO extra_script callback policy:
# - Pre-build: always run audits using the configured policy
#   (hard-fail when enabled, otherwise soft-fail).
# - Post-build: run audits only when hard-fail is disabled, in soft-fail mode,
#   so findings are still visible at the end of the build.
# - Note: post-build is attached to the .bin output action and may not trigger
#   on cache hits; soft mode also runs in pre-build so audits still execute.


def read_hard_fail_setting(project_dir: str) -> bool:
    """Return whether audit violations should fail the build."""
    config_path = os.path.join(project_dir, "config", "audit.ini")
    if not os.path.exists(config_path):
        return False

    parser = configparser.ConfigParser()
    parser.read(config_path)

    # hard_fail is the primary key; strict/fail_mode are accepted synonyms.
    if parser.has_option("audit", "hard_fail"):
        return parser.getboolean("audit", "hard_fail", fallback=False)

    if parser.has_option("audit", "strict"):
        return parser.getboolean("audit", "strict", fallback=False)

    if parser.has_option("audit", "fail_mode"):
        return parser.get("audit", "fail_mode", fallback="soft").strip().lower() == "hard"

    return False


def run_audits(project_dir: str, *, hard_fail: bool, stage: str) -> bool:
    """Run coding standard audits and return True when any check fails."""
    print(">>>")
    print(">>> NightDriver Coding Standard Audits")
    print(f">>> Stage: {stage}")
    print(f">>> Audit policy: {'hard-fail' if hard_fail else 'soft-fail'}")
    print(">>>")

    failed = False
    for script_name in AUDIT_SCRIPTS:
        script = os.path.join(project_dir, "tools", script_name)
        print(f"  Auditing: {script_name}...", end="", flush=True)

        # Keep logs quiet unless a check fails.
        result = subprocess.run([sys.executable, script], cwd=project_dir, capture_output=True, text=True)

        if result.returncode != 0:
            print(" FAILED!")
            print("-" * 40)
            print(result.stdout)
            print(result.stderr)
            print("-" * 40)
            failed = True
        else:
            print(" PASSED")

    print(">>>")
    if failed:
        print(">>> Audits completed with violations found.")
    else:
        print(">>> Audits completed successfully.")
    print(">>>")

    return failed


def _project_dir_from_env() -> str:
    if env is not None:
        return env.subst("$PROJECT_DIR")
    return os.getcwd()


def on_pre_build(source, target, env):
    project_dir = _project_dir_from_env()
    hard_fail = read_hard_fail_setting(project_dir)

    failed = run_audits(project_dir, hard_fail=hard_fail, stage="pre-build")
    if failed and hard_fail:
        print("!!! Build aborted due to coding standard violations (hard-fail enabled). !!!")
        sys.exit(1)


def on_post_build(source, target, env):
    project_dir = _project_dir_from_env()
    hard_fail = read_hard_fail_setting(project_dir)

    # Strict mode already enforced checks in pre-build.
    if hard_fail:
        return

    run_audits(project_dir, hard_fail=False, stage="post-build")


if env is not None:
    # Pre-build runs unconditionally at script-load time so findings are always
    # shown at least once, even on cache hits where no files are rebuilt.
    on_pre_build(None, None, None)
    env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", on_post_build)


if __name__ == "__main__":
    # Standalone helper mode for manual invocation.
    project_dir = os.getcwd()
    hard_fail = read_hard_fail_setting(project_dir)
    failed = run_audits(project_dir, hard_fail=hard_fail, stage="manual")
    if failed and hard_fail:
        sys.exit(1)
