# hyperfused

A networked fuse mounting daemon that runs over tcp or stdin/stdout.

## Install

To install it first make sure you have fuse installed.

* On Linux/Ubuntu `sudo apt-get install libfuse-dev`
* On OSX install [OSXFuse](http://osxfuse.github.com/) and pkg-config, `brew install pkg-config`

Then simply clone this repo and run

```
make install # you might need to sudo this
```

To verify that it is installed do

```
hyperfused --help # should print out some help
```

## Install from npm

You can also install it from npm by doing

```
npm install -g hyperfused
```

## Clients

Currently you can interact with this deamon using node.js by using the [hyperfuse](https://github.com/mafintosh/hyperfuse) module.

## Wire protocol

TBA

## License

MIT
