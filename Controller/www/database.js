let dbVersion = 11;

const dbName = "FireFly-Controller";
let db = new Dexie(dbName);

var controller_products = [{"pid":"FFC3232-2305","inputs":{"count":32},"outputs":{"count":32}},{"pid":"FFC0806-2305","inputs":{"count":8},"outputs":{"count":6}}];
var colors = [{"name":"Blue","hex":"#2a36e5"},{"name":"Green","hex":"#46a046"},{"name":"Red","hex":"#db1a1a"},{"name":"White","hex":"#ffffff"},{"name":"Yellow","hex":"#ffbc47"}];
var relay_models = [{"type":"BINARY","manufacturer":"Crydom","model":"DR2260D20V","description":"Contactor"},{"type":"BINARY","manufacturer":"Crydom","model":"DR2220D20U","description":"Relay"},{"type":"VARIABLE","manufacturer":"Crydom","model":"PMP2425W","description":"Proportional Control"}];
var circuit_icons = [{"name":"Chandelier","icon":"mdi:chandelier"},{"name":"Recessed Light","icon":"mdi:light-recessed"},{"name":"Floor Lamp","icon":"mdi:floor-lamp"},{"name":"Wall Sconce","icon":"mdi:wall-sconce"}]
var areas = [{"name":"Living Room"},{"name":"Garage"},{"name":"Kitchen"},{"name":"Hallway"},{"name":"Primary Bedroom"},{"name":"Guest Bedroom"}];

const initDB = () => {
    db.version(dbVersion).stores({
        colors: "++id, name", 
        controllers: "++id, &uuid, name, product",
        areas: "++id, name",
        circuit_icons: "++id, name",
        controller_products: "++id, pid",
        tags: "++id, &name",
        certificates: "++id, &certificate, commonName",
        breakers: "++id, name",
        circuits: "++id, name, breaker, area, icon, relay_model",
        relay_models: "++id, [manufacturer+model], is_custom",
        clients: "++id, name, &uuid",
        settings: "&setting, value"
    });

    db.on("populate", function(transaction) {
        transaction.controller_products.bulkAdd(controller_products);
        transaction.colors.bulkAdd(colors);
        transaction.relay_models.bulkAdd(relay_models);
        transaction.circuit_icons.bulkAdd(circuit_icons);
        transaction.areas.bulkAdd(areas);
    });

    db.open();
};