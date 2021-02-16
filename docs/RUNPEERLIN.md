# Running a Peer Node on Ubuntu 18.04 (LTS)

These instructions summarize the minimum number of steps to run a [peer node](https://nemtech.github.io/concepts/node.html#id1) and connect it to an existing network using the catapult-server build.

## Prerequisites

* Have built catapult-server following either [Conan](BUILD-conan.md) or [manual](BUILD-manual.md) instructions.
* Have defined the network nemesis block. Follow [these instructions](RUNNETWORKLIN.md) to run a private network.

## Replace the network configuration

> **NOTE:**
> This step is only required when connecting a peer node to a network that is up and running.
If you have not launched a network yet, move directly to ["Edit the node properties"](#edit-the-node-properties).

1. Download a copy of the following files and folders from the node that originated the network:

   * ``catapult-server/_build/resources/``
   * ``catapult-server/_build/seed/``

2. Save the downloaded files from the candidate peer node server in a new folder named ``network-config`` under the ``catapult-server/_build`` directory.

3. To add the network configuration to the peer node, run the following commands from the ``catapult-server/_build`` directory.

   ```sh
   mkdir seed
   cp -r network-config/seed/network-test/ seed/
   cp -r network-config/resources/ resources/
   ```

## Edit the node properties

The file ``resources/config-node.properties`` defines the node configuration.
Learn more about each network property in [this guide](https://nemtech.github.io/guides/network/configuring-node-properties.html#properties).

Open ``resources/config-node.properties`` and search for the ``[localnode]`` section.
Then, edit the properties with the node details. You will need at least these properties:

* ``host``: IP address or domain name of your node.
* ``friendlyName``: Name of your node for display purposes.
* ``version``: Version of catapult-server used by your node. Leave empty to use the current one.
* ``roles``: A comma-separated list of the following values:
  * ``Peer``: Node verifies transactions and blocks, runs the consensus algorithm, creates new blocks ([Documentation](https://docs.symbolplatform.com/concepts/node#peer-node)).
  * ``Api``: Node provides REST access to the blockchain ([Documentation](https://docs.symbolplatform.com/concepts/node#api-node)).
  * ``Voting``: Node participates in the Finalization process ([Documentation](https://docs.symbolplatform.com/concepts/block#finalization)).
  * ``IPv4``, ``IPv6``: IP version supported by the node. If neither is specified then version 4 is assumed.

If ``host`` or ``roles`` is incorrect other nodes won't be able to connect to this one.

For example:

``` ini
[localnode]
host = <YOUR_NODE_IP>
friendlyName = myPeerNode
version = 0.10.0.4
roles = IPv4,Peer
```

## Enable harvesting

This step enables [harvesting](https://nemtech.github.io/concepts/harvesting.html), which allows the node to produce new blocks.

> **NOTE:**
> At least one node of the network must have harvesting enabled to produce new blocks. If you don't want to enable harvesting, move directly to [Add other peer nodes](#add-other-peer-nodes).

1. Open the file ``resources/config-extensions-server.properties`` and enable the harvesting extension.

    ```ini
    # p2p extensions
    ...
    extension.harvesting = true
    ...
    ```

2. Edit the file ``resources/config-harvesting.properties``.

    ```ini
    [harvesting]
    harvesterSigningPrivateKey = <HARVESTER_SIGNING_PRIVATE_KEY>
    harvesterVrfPrivateKey = <HARVESTER_VRF_PRIVATE_KEY>
    enableAutoHarvesting = true
    ...
    ```

   * Replace ``<HARVESTER_SIGNING_PRIVATE_KEY>`` with the private key of an [eligible account](https://nemtech.github.io/concepts/harvesting.html#eligibility-criteria).

   * Replace ``<HARVESTER_VRF_PRIVATE_KEY>`` with the private key linked to the harvester account to randomize the block production. The link could be defined in the [nemesis block](RUNNETWORKLIN.md#append-the-vrf-keys-to-the-nemesis-block) or at a later point by announcing a **VRFKeyLinkTransaction** with the [CLI](https://github.com/nemtech/symbol-cli/blob/gh-pages/0.20.3.md#vrfkeylink) or [SDKs](https://github.com/nemtech/symbol-sdk-typescript-javascript).

## Generate TLS certificates

Catapult uses TLS 1.3 to provide secure connections and identity assurance among all nodes.

1. To generate and self sign the TLS certificate, you can download and run the script [cert-generate.sh](https://github.com/tech-bureau/catapult-service-bootstrap/blob/master/common/ruby/script/cert-generate.sh) from the ``catapult-server/_build`` directory.

    ```sh
    mkdir certificate
    cd certificate
    curl https://raw.githubusercontent.com/tech-bureau/catapult-service-bootstrap/master/common/ruby/script/cert-generate.sh --output cert-generate.sh
    chmod 777 cert-generate.sh
    ./cert-generate.sh
    ```

2. Open ``resources/config-user.properties`` and make sure that ``certificateDirectory`` points to the directory where the TLS certificates are being stored.

    ```ini
    [storage]

    seedDirectory = ../seed
    dataDirectory = ../data
    certificateDirectory = ../certificate
    pluginsDirectory = .
    ...
    ```

## List known peer nodes

The file ``resources/peers-p2p.json`` should list strong nodes to serve as beacons.
A random subset of beacons should be set in each node's peer file for best network performance.

> **NOTE:**
> If you just created the network and your node is the only one, make sure to add it to this file.

1. Open ``resources/peers-p2p.json`` and replace the public key and host with the public key, host, and port of the node that originated the network.

    ```json
    {
        "_info": "this file contains a list of all trusted peers and can be shared",
        "knownPeers": [
            {
                "publicKey": "0000000000000000000000000000000000000000000000000000000000000000",
                "endpoint": {
                "host": "127.0.0.1",
                "port": 7900
                },
                "metadata": {
                    "name": "peernode",
                    "roles": "Peer"
                }
            }
        ]
    }
    ```

    To get the node public key, run the following command in the folder that contains the node's certificates:

    ```sh
    openssl pkey -pubin -in ca.pubkey.pem -noout -text | openssl dgst -sha256 -hex
    ```

2. If the network has more peer nodes, you can add them to the ``knownPeers`` array.

## Run the node

```sh
cd bin
./catapult.server
```

If this is the first node of the network, you should see in the terminal the peer node producing new blocks:

```sh
... successfully harvested block at 1804 with signer ...
```

If you are connecting to an existing network, you should see the peer node synchronizing blocks:

```sh
... peer returned 42 blocks (heights 2 - 43)
```

The node can be stopped by pressing ``Ctrl-C`` and restarted simply by running ``catapult.server`` again.

## Check that the node is accessible and running

Finally, while the node is running and producing log output, open another terminal and move into the `_build/bin` folder. Then run `catapult.tools.health` to connect to the running node and retrieve some statistics:

```sh
cd _build/bin
./catapult.tools.health
```

Among other things, you should see a line reporting the current chain height:

```sh
... peernode @ 127.0.0.1:7900 [P2P] at height 118 (78 finalized) with score ...
```

The health tool connects to all nodes listed in the ``resources/peers-p2p.json`` so make sure you have added your own node to the list.
