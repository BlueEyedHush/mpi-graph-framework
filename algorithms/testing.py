
def _test(actual, expected):
    if(actual == expected):
        return (True, "")
    else:
        return (False, "Actual: {}, Expected: {}".format(actual, expected))

def run_tests(test_list):
    for id, t in zip(range(0, len(test_list)), test_list):
        actual, expected = t()
        success, msg = _test(actual, expected)
        if success:
            print("Test {} succeeded".format(id))
        else:
            print("Test {} failed: {}".format(id, msg))
