Import("env")

try:
    import intelhex
except ImportError:
    env.Execute("$PYTHONEXE -m pip install intelhex")