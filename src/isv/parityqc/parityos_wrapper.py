# ParityQC © 2025. See LICENSE.txt in /src/isv/parityqc/ for details.
"""
Auxiliary script to conveniently use parityos functionality from parityos.cpp via the python API.
"""

# TODO: Make sure parityos is installed whenever this script is executed.

from functools import wraps

# TODO: it would be better to explicitly signal an error. E.g. by sending back some special object
# which possibly contains an error message.
def exceptions_to_none(func):
    """The functions we call from C code must not raise any exceptions. Otherwise the whole process
    just terminates. We have to avoid this, even at the cost of worse error reporting.

    This is also the reason why we import all but the builtin packages within the function body.
    """
    @wraps(func)
    def wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except Exception as e:
            print("ERROR from wrapper script:", e)
            return None
        except:
            # just in case
            print("ERROR from wrapper script: ???")
            return None

    return wrapper


# TODO: this is just toy code for the fake_backend. The actual parityos backend does not need this.
_submission_results: dict[int, str] = dict()


@exceptions_to_none
def create_parityos_client(base_url: str) -> "HTTPClient":
    """Create the parityos http client.

    If the creation succeeds this implies that authentication worked.

    It is expected that the username and password are set via an environment variable. If anything
    goes wrong it returns `None` otherwise the client.
    """
    from parityos.services.authentication import EnvVarAuth
    from parityos.services.client import HTTPClient

    auth = EnvVarAuth()
    client = HTTPClient(authenticator=auth, host_url=base_url)
    return client


# TODO: this is still a fake implementation. The real one would use the client.
@exceptions_to_none
def submit_job(client: "HTTPClient", program: str) -> int :
    """Submit a program to the parityos web service to be compiled to a circuit.

    It returns the submission id which can be used to identify the job results once they are
    available. Returns `None` if anything goes wrong.
    """
    assert client is not None, "missing client"

    submission_id = _next_submission_id()
    _fake_backend(program, submission_id)

    return submission_id


@exceptions_to_none
def poll_result(submission_id: int) -> str | None:
    """Return the compilation result of a job or `None` if not available.

    NOTE: If the result is available it is removed from the global dict holding the results. This
    implies that if the result is never polled a memory leak can occur.

    NOTE: Right now it also returns `None` if anything goes wrong.
    """
    global _submission_results
    if submission_id in _submission_results:
        result = _submission_results[submission_id]
        del _submission_results[submission_id]
        return result
    else:
        return None


_gen = None

def _next_submission_id():
    global _gen

    def make_gen():
        i = 1 # arbitrary strictly positive number
        while True:
            yield i
            i += 1

    if _gen is None:
        _gen = make_gen()

    return next(_gen)


def _fake_backend(program: str, submission_id) -> None:
    """Placeholder for the calls to the parity api.

    The actual implementation would probably need to fire up an async computation to call the
    different core services and assemble a QAOA circuit from the outputs.
    """
    global _submission_results
    result = None

    # Tiny heuristic validation.
    assert "ProblemRepresentation" in program, "invalid input (expected problem representation)"

    # placeholder for the actual result.
    result = """{ "_cls": "Circuit", "_data": {} }"""

    _submission_results[submission_id] = result
