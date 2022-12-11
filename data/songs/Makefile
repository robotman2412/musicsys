
all: $(shell find . -name '*.m4a' | sed 's/m4a/mp3/')

%.mp3: %.m4a
	ffmpeg -i $< $@
