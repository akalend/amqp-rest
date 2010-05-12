all:
	g++ -ggdb -o amqp-rest main.cpp -levent -lrabbitmq -lrabbitmq++ -L/usr/local/lib -I/usr/local/includes -I../librabbitmq++ -L../librabbitmq++/rabbitmq++/build/Release/ -Iamqpcpp

re2c:
	re2c -o amqp_conf.c amqp_conf.re.c 

run:
	./amqp-rest n.conf
