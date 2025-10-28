# ParityQC © 2025. See LICENSE.txt in /src/isv/parityqc/ for details.
"""
Auxiliary script to conveniently use parityos functionality from parityos.cpp via the python API.
"""

# TODO: Make sure parityos is installed whenever this script is executed.

from typing import Optional

# TODO: this is just toy code for the fake_backend. The actual parityos backend does not need this.
submission_results: dict[int, str] = dict()


def create_parityos_client(username: str, base_url: str) -> Optional["HTTPClient"]:
    """Create the parityos http client.

    If the creation succeeds this implies that authentication worked.

    It is expected that the password is set via an environment variable. If anything goes wrong it
    returns `None` otherwise the client.
    """
    try:
        from parityos.services.client import HTTPClient
        client = HTTPClient(username=username, host=base_url)
        return client
    except:
        return None


# TODO: this is still a fake implementation. The real one would use the client.
def submit_job(client: "HTTPClient", program: str) -> int | None:
    """Submit a program to the parityos web service to be compiled to a circuit.

    It returns the submission id which can be used to identify the job results once they are
    available. Returns `None` if anything goes wrong.
    """
    try:
        assert client is not None, "missing client"
        import json
        parsed = json.loads(program)
        assert "content" in parsed
        content = parsed["content"]

        submission_id = _next_submission_id()
        _fake_backend(content, submission_id)
        # TODO: actual remote call will probably take some time but be non-blocking. Caller has to
        # actively poll for the result (`get_result`).

        return submission_id
    except:
        return None


# TODO: this is still a fake implementation.
def get_result(submission_id: int) -> str | None:
    """Return the compilation result of a job or `None` if not available.

    Right now it also returns `None` if anything goes wrong.
    """
    try:
        global submission_results
        if submission_id in submission_results:
            return submission_results[submission_id]
        else:
            return None
    except:
        return None


_gen = None

def _next_submission_id():
    global _gen

    def make_gen():
        i = 42 # arbitrary strictly positive number
        while True:
            yield i
            i += 1

    if _gen is None:
        _gen = make_gen()

    return next(_gen)


def _fake_backend(content: str, submission_id) -> None:
    try:
        global submission_results
        result = None

        if content == "hello":
            result = "is english"
        elif content == "hallo":
            result = "is german"
        else:
            result = "invalid input"

        submission_results[submission_id] = result
    except:
        return # fatal error
