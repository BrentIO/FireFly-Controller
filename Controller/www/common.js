function eventHandler(e) {
    console.log(`Event: ${e}`);
}


function errorHandler(e) {
    console.error(`Error: ${e}`);
}


function warningHandler(e) {
    console.warn(`Warning: ${e}`);
}


function loadMenu(){
    fetch("./menu.html")
    .then((response) => {
        if (response.ok)
            return response.text();
        else {
            throw new Error(response.statusText);
        }
    })
    .then((body) => {
        document.getElementById('menubar').innerHTML = body;
    })
    .catch(error => {
        console.error("Cannot load menu \n" + error);
    });
}


async function exportConfig(){
    try {
        const blob = await db.export({prettyJson: true});
        download(blob, `FireFlyConfig_${Date.now()}.json`, "application/json");
    } catch (error) {
        errorHandler(error);
    }
}


async function importConfig(file){
    try {
        await db.delete();
        db = await Dexie.import(file);
    } catch (error) {
        errorHandler(error);
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
        const record = JSON.parse(localStorage.getItem("controller_" + this.id));

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
        localStorage.setItem("controller_" + this.id, JSON.stringify(this));
    }

    delete(){
        localStorage.removeItem("controller_" + this.id);
    }

}
}