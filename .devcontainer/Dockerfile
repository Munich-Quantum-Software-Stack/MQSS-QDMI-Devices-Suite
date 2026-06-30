FROM --platform=x86_64 python:3.11-bookworm

ENV TZ="Europe/Berlin"

RUN apt-get update -y && \
    apt-get autoremove -y
RUN apt-get install -y software-properties-common bash-completion cmake build-essential iputils-ping rsync p7zip-full libuv1-dev git libcjson-dev nlohmann-json3-dev librabbitmq-dev
RUN pip install myqlm-interop[qiskit,cirq,pyquil] flask

WORKDIR /home

CMD ["bash"]
