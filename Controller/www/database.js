let dbVersion = 3;

const dbName = "FireFly-Controller";
let db = new Dexie(dbName);


const initDB = () => {
    db.version(dbVersion).stores({
        colors: "name", 
        controllers: "++id, &uuid, name",
        areas: "name"
    })
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

    constructor(uuid){
        this.uuid = uuid;
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
        const record = JSON.parse(localStorage.getItem(this.uuid));

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
        localStorage.setItem(this.uuid, JSON.stringify(this));
    }

    delete(){
        localStorage.removeItem(this.uuid);
    }

}