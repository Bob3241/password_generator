all: ./build/password_generator
	@mkdir -p build
	@cd ./build ; cmake .. ; make

run: all
	@./build/password_generator
