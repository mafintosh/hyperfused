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

## Clients

Currently you can interact with this deamon using node.js by using the [hyperfuse](https://github.com/mafintosh/hyperfuse) module.

## Wire protocol

TBA

## License

MIT
