(function prepareAccountRestrictionsCollection() {
	db.createCollection('accountRestrictions');
	db.accountRestrictions.createIndex({ 'accountRestrictions.address': 1 }, { unique: true });

	db.accountRestrictions.getIndexes();
})();
