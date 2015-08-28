
# Utility functions

# Takes a string and return True if it can be converted to a float
# returns False otherwise 
def is_float(x):
    try:
        float(x)
    except (ValueError, TypeError):
        return False
    else:
        return True

# Takes a string and return True if it can be converted to an int
# returns False otherwise 
def is_int(x):
    try:
        int(x)
    except (ValueError, TypeError):
        return False
    else:
        return True

# Takes a string and return a float if it can be converted to a float
# returns an int if it can be converted to an int
def dyn_cast(val):
    if is_float(val):
        return float(val)
    elif is_int(val):
        return int(val)
    else:
        return val

