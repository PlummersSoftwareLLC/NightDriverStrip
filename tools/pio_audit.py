import subprocess
import sys
import os

# PlatformIO extra_script (pre:) runs during environment construction.
# To make this a mandatory gate that fails the build, we run the audits 
# immediately.

# In SCons context, __file__ is not defined. 
# PIO defines PROJECT_DIR in the environment.
# But we can also use os.getcwd() as PIO runs from project root.

project_dir = os.getcwd()

print(">>> NightDriver Coding Standard Audits")

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
    print("!!! Build aborted due to coding standard violations. !!!")
    sys.exit(1)

print(">>> Audits completed successfully.")
