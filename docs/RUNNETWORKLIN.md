# Running a Private Network on Ubuntu 18.04 (LTS)

The instructions below describe the minimum amount of changes to run a network from the catapult-server build.

NOTE: Replace ``mijin-test`` occurrences with the network type selected.
The possible network values are: mijin (private), mijin-test (private testnet), public, public-test.

## Prerequisites

* Follow [catapult-server build](BUILDLIN.md) instructions.

## Copy the configuration template

After building catapult-server, copy the configuration templates from the root folder of the repository under the ``_build`` directory.

```sh
cd catapult-server/_build
cp ../resources/* resources/
cp ../tools/nemgen/resources/mijin-test.properties resources/
```

WARNING: It is NOT recommended to use the default configuration values in production environments.

## Generate accounts

Generate a set of accounts. The accounts stored in the file ``nemesis.addresses.txt`` will be used at a later step to receive the network's currency and harvest mosaics on the first block.

```sh
./bin/catapult.tools.address -g 10 --network mijin-test > nemesis.addresses.txt
cat nemesis.addresses.txt
```

The script generates ten accounts for the nemesis block, but the number of accounts is customizable.

## Create the seed and txes directory

1\. Create a directory to save the generated nemesis block under ``catapult-server/_build``.

```sh
mkdir -p seed/network-test/00000
```
2\. Create a directory to save additional transactions embedded in the nemesis block.

```sh
mkdir txes
```

## Edit the nemesis block

catapult-server calls the first block in the chain the nemesis block.
The first block is defined before launching a new network and sets the initial distribution of mosaics.

The file ``resources/mijin-test.properties`` defines the transactions issued in the nemesis block.

1\. Open ``mijin-test.properties`` and edit the ``[nemesis]`` section.
Replace ``nemesisGenerationHashSeed`` with a unique SHA3-256 hash that will identify the network
and ``nemesisSignerPrivateKey`` with a private key from ``nemesis.addresses.txt``.

```ini
[nemesis]

networkIdentifier = mijin-test
nemesisGenerationHashSeed = 57F7DA205008026C776CB6AED843393F04CD458E0AA2D9F1D5F31A402072B2D6
nemesisSignerPrivateKey = 0000000000000000000000000000000000000000000000000000000000000000
```

2\. Replace ``[cpp]`` and ``[output]`` sections with the following configuration.

```ini
[cpp]

cppFileHeader =

[output]

cppFile =
binDirectory = ../seed/network-test
```

3\. Edit the ``[distribution]`` section.
The accounts defined under the distribution list will receive mosaics units.
Replace at least one address of each group with an address from ``nemesis.addresses.txt``.
The total amount of units distributed must match the original mosaic definition.

WARNING: Do not add the ``nemesisSignerPrivateKey`` account to the distribution list.
The nemesis signer account cannot announce or participate in transactions.
The mosaics received by the nemesis account will be lost forever.

Here is an example of how the distribution list looks like after replacing some addresses:

```ini
[distribution>cat:currency]

SAIBNNG7QJXY54Z334HOKA36NTH7FRRCKFRM4XY = 409'090'909'000'000
...

[distribution>cat:harvest]

SBMEZB54VXTH4PRAUJJQJJFB2SNQZQ2SUI6J7BA = 1'000'000
...
```

Your addresses should be different, as ``catapult.tools.address`` generates different accounts after every execution. Make sure that someone holds the private keys associated with every address listed before the network is launched.

4\. Edit the ``[transactions]`` section.

```ini
[transactions]

transactionsDirectory = ../txes
```

5\. Continue editing the nemesis block properties to fit your network requirements and save the configuration before moving to the next step.

## Edit the network properties

The file ``resources/config-network.properties`` defines the network configuration.
Learn more about each network property in [this guide](https://nemtech.github.io/guides/network/configuring-network-properties.html#properties).

Edit the properties file to match the nemesis block with the desired network configuration.

NOTE: By default, ``initialCurrencyAtomicUnits`` and ``totalChainImportance`` do not match the values set in ``./resources/mijin-test.properties``.
Other values that might differ from the nemesis block file are ``identifier``, ``nemesisSignerPublicKey``, ``generationHashSeed``, ``harvestNetworkFeeSinkAddress``, ``mosaicRentalFeeSinkAddress``, and ``namespaceRentalFeeSinkAddress``.

## Append the VRF Keys to the nemesis block

The process of creating new blocks is called [harvesting](https://nemtech.github.io/concepts/harvesting.html).
Each node of the network can host zero or more harvester accounts to create new blocks and get rewarded.

In order to be an eligible harvester, the account must:

1\. Own a certain amount of harvesting mosaics defined in ``config-network.properties`` between ``minHarvesterBalance`` and ``maxHarvesterBalance``.

2\. Announce a valid VrfKeyLinkTransaction. The VRF transaction links the harvester account with a second key pair to randomize block production and leader selection.

In order to ensure that the network produces a second block after its launch, the nemesis block must include at least one valid VrfKeyLinkTransaction linking a harvester account with a second key pair.

Run the linker tool to create a VrfKeyLinkTransaction. 

```sh
cd bin
./catapult.tools.linker --resources ../ --type vrf --secret <HARVESTER_PRIVATE_KEY> --linkedPublicKey <VRF_PUBLIC_KEY> --output ../txes/tx0.bin
```

* Replace ``<HARVESTER_PRIVATE_KEY>`` with the private key of an account that has received sufficient harvesting mosaics in ``resources/mijin-test.properties`` ``[distribution>cat:harvest]``.

* Replace ``<VRF_PUBLIC_KEY>`` with the public key of an unused account from ``nemesis.addresses.txt``.

## Generate the network mosaic ids

The network mosaic ids are autogenerated based on the configuration provided in the file ``resources/mijin-test.properties``.

1\. Run the nemesis block generator.

```sh
./catapult.tools.nemgen --nemesisProperties ../resources/mijin-test.properties
```

2\. Copy the currency and harvest mosaic ids displayed on the command line prompt.

```sh
(nemgen::NemesisConfigurationLoader.cpp@57) Mosaic Summary
(nemgen::NemesisConfigurationLoader.cpp@32)  - cat:currency (621EC5B403856FC2)
...
(nemgen::NemesisConfigurationLoader.cpp@32)  - cat:harvest (4291ED23000A037A)
```
3\. Set ``currencyMosaicId`` and ``harvestingMosaicId`` in ``resources/mijin-test.properties`` with the values generated in the previous step.

```ini
[chain]

currencyMosaicId = 0x621E'C5B4'0385'6FC2
harvestingMosaicId = 0x4291'ED23'000A'037A
```

## Generate the nemesis block

1\. Run the nemesis block generator a second time, this time with the correct mosaic ids values.

```sh
./catapult.tools.nemgen --nemesisProperties ../resources/mijin-test.properties
```

2\. Copy the generated nemesis block under the ``data`` folder.

```sh
cd ..
cp -r seed/network-test/* data/
```

## Configure the node

Follow the [next guide](RUNPEERLIN.md) to configure a peer node and start the network.
