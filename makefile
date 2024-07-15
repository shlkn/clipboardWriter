target:
	g++ clipboardWriter.cpp -lX11 -o clipboardWriter

clean:
	rm clipboardWriter

run:
	./clipboardWriter "sample sting"

runWithFile:
	./clipboardWriter clipboardWriter.cpp