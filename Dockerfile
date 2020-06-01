FROM ubuntu:20.04

WORKDIR /usr/src/app

# COPY helloworld .
# CMD ["./helloworld"]

# Somehow this doesn't work...
# COPY ./bazel-bin/src/client/greeter_client .

COPY greeter_client .
# CMD ["./greeter_client"]

# COPY file.sh .
# ENTRYPOINT ["./file.sh"]

ENTRYPOINT ["./greeter_client"]
