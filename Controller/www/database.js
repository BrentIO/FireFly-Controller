let dbVersion = 8;

const dbName = "FireFly-Controller";
let db = new Dexie(dbName);

let product_ids = [{"pid":"FFC3232-2305","inputs":{"count":32},"outputs":{"count":32}},{"pid":"FFC0806-2305","inputs":{"count":8},"outputs":{"count":6}}];
let colors = [{"name":"Blue","hex":"#2a36e5"},{"name":"Green","hex":"#46a046"},{"name":"Red","hex":"#db1a1a"},{"name":"White","hex":"#ffffff"},{"name":"Yellow","hex":"#ffbc47"}];


const initDB = () => {
    db.version(dbVersion).stores({
        colors: "name", 
        controllers: "++id, &uuid, name",
        areas: "name",
        product_ids: "pid",
        tags: "name"
    });

    db.version(1).upgrade(function(transaction) {
        transaction.product_ids.clear();
        transaction.product_ids.bulkAdd(product_ids);
    });

    db.on("populate", function(transaction) {
        transaction.product_ids.bulkAdd(product_ids);
        transaction.colors.bulkAdd(colors);
    });

    db.open();
};


async function exportConfig(){
    try {
        const blob = await db.export({prettyJson: true});
        download(blob, `FireFlyConfig_${Date.now()}.json`, "application/json");
    } catch (error) {
        eventHandler(error);
    }
}


async function importConfig(file){
    try {
        await db.delete();
        db = await Dexie.import(file);
    } catch (error) {
        eventHandler(error);
    }
}


class controllerLocalStorage{
    ip;
    certificates = [];
    firmwareVersion;
    uiVersion;
    visualToken;
    uuid;

    constructor(id){
        this.id = id;
        this.retrieve();
    }

    get ip(){
        return this.ip;
    }

    set ip(value){
        this.ip = value;
    }

    get certificates(){
        return this.certificates;
    }

    set certificates(value){
        this.certificates = value;
    }

    get firmwareVersion(){
        return this.firmwareVersion;
    }

    set firmwareVersion(value){
        this.firmwareVersion = value;
    }

    get uuid(){
        return this.uuid;
    }

    set uuid(value){
        this.uuid = value;
    }

    get uiVersion(){
        return this.uiVersion;
    }

    set uiVersion(value){
        this.uiVersion = value;
    }

    get visualToken(){
        return this.visualToken;
    }

    set visualToken(value){
        this.visualToken = value;
    }

    retrieve(){
        const record = JSON.parse(localStorage.getItem(this.id));

        if(record == null){
            return;
        }

        if("ip" in record){
            this.ip = record.ip;
        }

        if("certificates" in record){
            this.certificates = record.certificates;
        }

        if("firmwareVersion" in record){
            this.firmwareVersion = record.firmwareVersion;
        }

        if("uiVersion" in record){
            this.uiVersion = record.uiVersion;
        }
    }

    save(){
        localStorage.setItem(this.id, JSON.stringify(this));
    }

    delete(){
        localStorage.removeItem(this.id);
    }

}