# Running a Private Network on Ubuntu 18.04 (LTS)

The instructions below describe the minimum amount of changes to run a network from the catapult-server build.

NOTE: Replace ``mijin-test`` occurrences for the network type selected.
The possible network values are: mijin (private), mijin-test (private testnet), public, public-test.

## Prerequisites

* Follow [catapult-server build](BUILDLIN.md) instructions.

## Copy the configuration template

After building catapult-server, copy the configuration templates from the root folder of the repository under the ``_build`` directory.

```sh
cd catapult-server/_build
cp ../resources/* resources/
cp ../../tools/nemgen/resources/mijin-test.properties ../resources/
```

NOTE: Copying the resources folder is NOT recommended to deploy a production network.

## Generate accounts

Generate a set of accounts. The accounts stored in the file ``nemesis.addresses.txt`` will be used at a later step to receive the network's currency and harvest mosaics on the first block.

```sh
./bin/catapult.tools.address -g 10 --network mijin-test > nemesis.addresses.txt
cat nemesis.addresses.txt
```
NOTE: The script is generating ten accounts for the nemesis block, but the number of accounts is customizable.

## Edit the nemesis block

The file ``resources/mijin-test.properties`` defines the transactions issued in the nemesis block.

1\. Open ``mijin-test.properties`` and edit the ``[nemesis]`` section.
Replace ``nemesisGenerationHash`` for a unique SHA3-256 hash that will identify the network
and ``nemesisSignerPrivateKey`` for one of the private keys generated in ``nemesis.addresses.txt``.

```ini
[nemesis]

networkIdentifier = mijin-test
nemesisGenerationHash = 57F7DA205008026C776CB6AED843393F04CD458E0AA2D9F1D5F31A402072B2D6
nemesisSignerPrivateKey = B59D53B625CC746ECD478FA24F1ABC80B2031EB7DFCD009D5A74ADE615893175
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
The account defined under the distribution list will receive mosaics units.
Replace at least an address for one of the generated addresses in ``nemesis.addresses.txt``.

```ini
[distribution>cat:currency]

SCHL3EXRBS7HNGZWM5YXPOWQRJPXVXHAEMISSOBP = 409'090'909'000'000

[distribution>cat:harvest]

SCHL3EXRBS7HNGZWM5YXPOWQRJPXVXHAEMISSOBP = 1'000'000
```
4\. Continue editing the nemesis block to fit the network requirements before moving to the next step.

NOTE: Do not add the ``nemesisSignerPrivateKey`` account to the distribution list.
The nemesis signer account will not be able to announce transactions.
The mosaics received by the nemesis account will be lost forever.

## Create the seed directory

Create a directory to save the generated nemesis block.

```ssh
mkdir -p seed/network-test/00000
dd if=/dev/zero of=seed/network-test/00000/hashes.dat bs=1 count=64
```
## Generate the network mosaic ids

1\. Run the nemesis block generator.

```ssh
cd bin
./catapult.tools.nemgen --nemesisProperties ../resources/mijin-test.properties
cd ..
```

2\. Copy the currency and harvest mosaic ids displayed on the command line prompt.

```ssh
(nemgen::NemesisConfigurationLoader.cpp@57) Mosaic Summary
(nemgen::NemesisConfigurationLoader.cpp@32)  - cat:currency (621EC5B403856FC2)
...
(nemgen::NemesisConfigurationLoader.cpp@32)  - cat:harvest (4291ED23000A037A)
```

## Edit the network properties

The file ``resources/config-network.properties`` defines the network configuration.
Learn more about each network property in [this guide](https://nemtech.github.io/guides/network/configuring-network-properties.html#properties).

Edit the properties file to match the nemesis block and with the desired network configuration.

NOTE: By default, ``initialCurrencyAtomicUnits`` and ``totalChainImportance`` do not match the values set in ``./resources/mijin-test.properties``.
Other values that might differ from the nemesis block file are ``identifier``,  ``publicKey``, ``generationHash``, ``currencyMosaicId``, ``harvestMosaicId``, ``mosaicRentalFeeSinkAddress``, and ``namespaceRentalFeeSinkAddress``.

## Generate the nemesis block

Run the nemesis block generator.

```ssh
cd bin
./catapult.tools.nemgen --nemesisProperties ../resources/mijin-test.properties
cd ..
cp -r /opt/catapult-server/seed/mijin-test/* data/
```

## Configure the node

Follow the [next guide](RUNPEERLIN.md) to configure a peer node and start the network.
