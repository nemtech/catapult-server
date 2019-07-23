(function prepareDbConfiguration() {
	db.setProfilingLevel(1, 100); // log slow queries
})();

(function prepareDbCollections() {
	function makeSparse(propertyName) {
		return { partialFilterExpression: { [propertyName]: { $exists: true } } };
	}

	function addCommonTransactionIndexes(collection) {
		collection.createIndex({ 'transaction.signer': 1, _id: -1 });
		collection.createIndex({ 'transaction.recipient': 1, _id: -1 });
		collection.createIndex({ 'meta.hash': 1 }, Object.assign({ unique: true }, makeSparse('meta.hash')));
		collection.createIndex({ 'meta.addresses': 1 }, makeSparse('meta.addresses'));
		collection.createIndex({ 'meta.aggregateId': 1 }, makeSparse('meta.aggregateId'));
	}

	function addTransactionCollection(collectionName) {
		db.createCollection(collectionName);
		addCommonTransactionIndexes(db[collectionName]);
		db[collectionName].createIndex({ 'meta.aggregateHash': 1 }, makeSparse('meta.aggregateHash'));
	}

	db.createCollection('blocks');
	db.blocks.createIndex({ 'block.signer': 1 });
	db.blocks.createIndex({ 'block.timestamp': -1 }, { unique: true });
	db.blocks.createIndex({ 'block.height': -1 }, { unique: true });
	db.blocks.createIndex({ 'block.signer': 1, 'block.height': -1 }, { unique: true });

	db.createCollection('transactions');
	addCommonTransactionIndexes(db.transactions);
	db.transactions.createIndex({ 'meta.height': -1 });
	db.transactions.createIndex({ 'transaction.deadline': -1 });
	db.transactions.createIndex({ 'transaction.cosignatures.signer': 1 }, makeSparse('transaction.cosignatures.signer'));
	db.transactions.createIndex({ 'transaction.mosaicId': 1, 'transaction.type': 1 }, makeSparse('transaction.mosaicId'));
	db.transactions.createIndex({ 'transaction.namespaceId': 1, 'transaction.type': 1 }, makeSparse('transaction.namespaceId'));

	db.createCollection('transactionStatements');
	db.transactionStatements.createIndex({ 'height': 1, 'source.primaryId': 1, 'source.secondaryId': 1 }, { unique: true });

	db.createCollection('addressResolutionStatements');
	db.addressResolutionStatements.createIndex({ 'height': 1, 'unresolved': 1 }, { unique: true });

	db.createCollection('mosaicResolutionStatements');
	db.mosaicResolutionStatements.createIndex({ 'height': 1, 'unresolved': 1 }, { unique: true });

	db.createCollection('accounts');
	db.accounts.createIndex({ 'account.publicKey': 1 }); // cannot be unique because zeroed public keys are stored
	db.accounts.createIndex({ 'account.address': 1 }, { unique: true });

	addTransactionCollection('unconfirmedTransactions');

	addTransactionCollection('partialTransactions');

	db.createCollection('transactionStatuses');
	db.transactionStatuses.createIndex({ hash: 1 }, { unique: true });
	db.transactionStatuses.createIndex({ deadline: -1 });

	db.blocks.getIndexes();
	db.transactions.getIndexes();
	db.transactionStatements.getIndexes();
	db.addressResolutionStatements.getIndexes();
	db.mosaicResolutionStatements.getIndexes();
	db.accounts.getIndexes();
	db.unconfirmedTransactions.getIndexes();
	db.partialTransactions.getIndexes();
	db.transactionStatuses.getIndexes();
})();

load('mongoAccountRestrictionsDbPrepare.js')
load('mongoLockInfoDbPrepare.js')
load('mongoMosaicDbPrepare.js')
load('mongoMultisigDbPrepare.js')
load('mongoNamespaceDbPrepare.js')
