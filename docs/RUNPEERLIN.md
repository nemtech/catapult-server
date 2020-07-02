# Running a Peer Node on Ubuntu 18.04 (LTS)

These instructions summarize the minimum number of steps to run a [peer node](https://nemtech.github.io/concepts/node.html#id1) and connect it to an existing network using the catapult-server build.

## Prerequisites

* Have defined the network nemesis block. Follow [these instructions](RUNNETWOWORKLIN.md) to run a private network.
* Have catapult-server compiled in the node. Follow [these instructions](BUILDING.md) to build catapult-server.

## Replace the network configuration

NOTE: This step is only required when connecting a peer node to a network that is up and running.
If you have not launched a network yet, move directly to ["Edit the node properties"](#edit-node-properties).

1\. Download a copy of the following files and folders from the node that originated the network:

* ``catapult-server/_build/resources/``
* ``catapult-server/_build/seed/``

2\. Save the downloaded files from the candidate peer node server in a new folder named ``network-config`` under the ``catapult-server/_build`` directory.

3\. To add the network configuration to the peer node, run the following commands from the ``catapult-server/_build`` directory.

```sh
mkdir data
cp -r network-config/seed/network-test/ data/
cp -r network-config/resources/ resources/
```

## Edit the node properties

The file ``resources/config-node.properties`` defines the node configuration. 
Learn more about each network property in [this guide](https://nemtech.github.io/guides/network/configuring-node-properties.html#properties).

Open ``resources/config-node.properties`` and search the ``[localnode]`` section.
Then, edit the properties with the peer node details.

``` ini
[localnode]
host = <YOUR_NODE_IP>
friendlyName = myPeerNode
version = 0
roles = Peer
```

## Enable harvesting

This step enables [harvesting](https://nemtech.github.io/concepts/harvesting.html), which allows the node to produce new blocks.

NOTE: At least one node of the network must have harvesting enabled to produce new blocks. If you don't want to enable harvesting, move directly to [Add other peer nodes](#add-other-peer-nodes).
 
1\. Open the file ``resources/config-extensions-server.properties`` and enable the harvesting extension.

```ini
# p2p extensions
...
extension.harvesting = true
...
```

2\. Edit the file ``resources/config-harvesting.properties``.

```ini
[harvesting]
harvesterSigningPrivateKey = ``<HARVESTER_SIGNING_PRIVATE_KEY>``
harvesterVrfPrivateKey = ``<HARVESTER_VRF_PRIVATE_KEY>`` 
isAutoHarvestingEnabled = true
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

dataDirectory = ../data
certificateDirectory = ../certificate
pluginsDirectory = .
...
```

## Add other peer nodes

The file ``resources/peers-p2p.json`` should list strong nodes to serve as beacons.
A random subset of beacons should be set in each node's peer file for best network performance.

1\. Open the file and replace the public key and host with the public key, host, and port of the node that originated the network.

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

2\. If the network has more peer nodes, you can add them to the ``knownPeers`` array.

## Run the node

```sh
cd bin
./catapult.server
```

If this is the first node of the network, you should see in the terminal the peer node producing new blocks:

```sh
2020-04-17 12:55:25.687924 0x00007f39d7a39700: <info> (src::ScheduledHarvesterTask.cpp@35) successfully harvested block
 at 1804 with signer 4F92FF92C3F8F71A17615223E978A77E7DADEF401EEB046F1F31DF7AC8345DDE
```

If you are connecting to an existing network, you should see the peer node synchronizing blocks:

```sh
2020-04-17 08:19:18.798634 0x00007f71c5976700: <info> (chain::ChainSynchronizer.cpp@206) peer returned 42 blocks (heights 2 - 43)
```
