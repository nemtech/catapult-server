# catapult-server

[![docs](badges/docs--green.svg)](https://nemtech.github.io)
[![docker](badges/docker-techbureau-brightgreen.svg)](https://hub.docker.com/u/techbureau)

Symbol-based networks rely on nodes to provide a trustless, high-performance, and secure blockchain platform.

These nodes are deployed using [catapult-server] software, a C++ rewrite of the previous Java-written [NEM] distributed ledger that has been running since 2015.

Learn more about the protocol by reading the [whitepaper] and the  [developer documentation].

## Package Organization

catapult-server code is organized as follows:

| Folder name | Description |
| -------------|--------------|
| /extensions | Modules that add features to the bare catapult-server. These capabilities are all optional because none of them impact consensus. |
| /external | External dependencies that are built with the server. |
| /plugins | Modules that introduce new and different ways to alter the chain's state via transactions. |
| /resources | Default properties. These can be configured per network and node. |
| /scripts | Utility scripts for developers. |
| /sdk | Reusable code used by tests and tools. |
| /seed | Nemesis blocks used in tests. |
| /src | Symbol's core engine. |
| /tests | Collection of tests. |
| /tools | Tools to deploy and monitor networks and nodes. |

## Building the Image

To compile catapult-server source code, follow the [developer notes](docs/README.md). 

## Running catapult-server

catapult-server executable can be used either to run different types of nodes or to launch new networks. This section contains the instructions on how to run the catapult-server for various purposes.

### Test Network Node

Developers can deploy test net nodes to experiment with the offered transaction set in a live network without the loss of valuable assets. 

To run a test net node, follow [this guide](https://nemtech.github.io/guides/network/running-a-test-net-node.html).

### Main Network Node

At the time of writing, the main public network has not been launched. Follow the latest updates about the public launch on the [Forum].

### Private Test Network

With Symbol, businesses can launch and extend private networks by developing custom plugins and extensions at the protocol level. The package [Service Bootstrap] contains the necessary setup scripts to deploy a network for testing and development purposes with just one command. 

To run a private test net, follow [this guide](https://nemtech.github.io/guides/network/creating-a-private-test-net.html#creating-a-private-test-net).

### Private Network
 
Instructions on how to launch a secure and production-ready private chain will be released here.

## Contributing

Before contributing please [read this](CONTRIBUTING.md).

## Getting Help

- [Symbol Developer Documentation][developer documentation]
- [Symbol Whitepaper][whitepaper]
- Join the community [slack group (#help)][slack] 
- If you found a bug, [open a new issue][issues]

## License

Copyright (c) 2018 Jaguar0625, gimre, BloodyRookie, Tech Bureau, Corp Licensed under the [GNU Lesser General Public License v3](LICENSE)

[developer documentation]: https://nemtech.github.io
[Forum]: https://forum.nem.io/c/announcement
[issues]: https://github.com/nemtech/catapult-server/issues
[slack]: https://join.slack.com/t/nem2/shared_invite/enQtMzY4MDc2NTg0ODgyLWZmZWRiMjViYTVhZjEzOTA0MzUyMTA1NTA5OWQ0MWUzNTA4NjM5OTJhOGViOTBhNjkxYWVhMWRiZDRkOTE0YmU
[catapult-server]: https://github.com/nemtech/catapult-server
[symbol-rest]: https://github.com/nemtech/catapult-rest
[Service Bootstrap]: https://github.com/tech-bureau/catapult-service-bootstrap
[nem]: https://nem.io
[whitepaper]: https://nemtech.github.io/catapult-whitepaper/main.pdf
