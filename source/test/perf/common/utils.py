def is_float(x):
    try:
        float(x)
    except (ValueError, TypeError):
        return False
    else:
        return True

def is_int(x):
    try:
        int(x)
    except (ValueError, TypeError):
        return False
    else:
        return True

def dyn_cast(val):
    if is_float(val):
        return float(val)
    elif is_int(val):
        return long(val)
    else:
        return val


