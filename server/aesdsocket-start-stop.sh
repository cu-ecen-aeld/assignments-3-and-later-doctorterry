#! /bin/sh

# MELP p457
case "$1" in
    start)
        echo "Starting simpelserver"
        start-stop-daemon -S -n aesdsocket -a /usr/bin/aesdserver -- -d
        ;;
    stop)
        echo "Stopping simpleserver"
        start-stop-daemon -K -n aesdsocket
        ;;
    *)
        echo "Usage: $0 {start|stop}"
    exit 1
esac

exit 0