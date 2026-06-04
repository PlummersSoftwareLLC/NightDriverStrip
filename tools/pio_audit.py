import configparser
import os
import subprocess
import sys

# PlatformIO extra_script (pre:) runs during environment construction.
# Audits are soft-fail by default to avoid blocking local or CI builds unless
# a local opt-in config enables hard-fail mode.

project_dir = os.getcwd()
audit_config_path = os.path.join(project_dir, "config", "audit.ini")


def read_hard_fail_setting(config_path: str) -> bool:
    """Return whether audit violations should fail the build."""
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


hard_fail = read_hard_fail_setting(audit_config_path)

print(">>> NightDriver Coding Standard Audits")
print(f">>> Audit policy: {'hard-fail' if hard_fail else 'soft-fail'}")

audit_scripts = [
    os.path.join(project_dir, "tools", "audit_globals_order.py"),
    os.path.join(project_dir, "tools", "audit_include_rules.py")
]

failed = False
for script in audit_scripts:
    script_name = os.path.basename(script)
    print(f"  Auditing: {script_name}...", end="", flush=True)

    # Run the script and capture output to keep the build log clean unless it fails
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

if failed:
    if hard_fail:
        print("!!! Build aborted due to coding standard violations (hard-fail enabled). !!!")
        sys.exit(1)
    print("!!! Coding standard violations detected, but continuing (soft-fail policy). !!!")

print(">>> Audits completed successfully.")
