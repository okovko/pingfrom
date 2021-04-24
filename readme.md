how to deploy

make

in one terminal: ./server 1234
in another terminal: ./client username@localhost:1234 localhost

you can press ctrl-c on the client (after waiting a few seconds)
to send SIGINT to the server's child process that ran ping via
execvp, and ping will complete normally and send all the output
to the client.

the signal handling code calls non async-signal-safe functions,
which means it will not work 100% of the time. i will improve this
later, it will require passing an argument to the client's signal
handler and using write instead of fwrite (although the output
can be inconsistent as a result of using write).

right now the parent process waits a few seconds before looking
for a symbolic message (just a single byte) from the client.
that's because otherise the parent process will eat part of the
client's message.

there are two ways to fix that off the top of my head. i can either
fork() after the client's message is received, or i can have the
parent process wait on a blocking read piped in from the child
process once the message has been received. latter way should be
better.
