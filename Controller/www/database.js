let dbVersion = 11;

const dbName = "FireFly-Controller";
let db = new Dexie(dbName);

let product_ids = [{"pid":"FFC3232-2305","inputs":{"count":32},"outputs":{"count":32}},{"pid":"FFC0806-2305","inputs":{"count":8},"outputs":{"count":6}}];
let colors = [{"name":"Blue","hex":"#2a36e5"},{"name":"Green","hex":"#46a046"},{"name":"Red","hex":"#db1a1a"},{"name":"White","hex":"#ffffff"},{"name":"Yellow","hex":"#ffbc47"}];
let relay_models = [{"type":"BINARY","manufacturer":"Crydom","model":"DR2260D20V","description":"Contactor"},{"type":"BINARY","manufacturer":"Crydom","model":"DR2220D20U","description":"Relay"},{"type":"VARIABLE","manufacturer":"Crydom","model":"PMP2425W","description":"Proportional Control"}];


const initDB = () => {
    db.version(dbVersion).stores({
        colors: "name", 
        controllers: "++id, &uuid, name",
        areas: "name",
        product_ids: "pid",
        tags: "name",
        certificates: "++id, &certificate",
        breakers: "++id, name",
        relays: "++id, name, breaker",
        relay_models: "++id, [manufacturer+model], is_custom"
    });

    db.version(1).upgrade(function(transaction) {
        transaction.product_ids.clear();
        transaction.product_ids.bulkAdd(product_ids);
    });

    db.on("populate", function(transaction) {
        transaction.product_ids.bulkAdd(product_ids);
        transaction.colors.bulkAdd(colors);
        transaction.relay_models.bulkAdd(relay_models);
    });

    db.open();
};