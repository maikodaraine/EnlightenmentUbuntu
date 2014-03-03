from Main import MainInterface

try:
    from efl import elementary
except ImportError:
    import elementary

elementary.init()
