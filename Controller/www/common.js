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


async function storeCertificate(filename, certificate){

    if(await db.certificates.where({certificate: certificate}).count() > 0){
        return;
    }

    truncatedCert = certificate.replace("-----BEGIN CERTIFICATE-----", "");
    truncatedCert = truncatedCert.replace("-----END CERTIFICATE-----", "");

    try {
        decoded = decodeCert(truncatedCert);

        await db.certificates.put({
            fileName: filename,
            certificate: certificate,
            commonName: decoded['commonName-X520'],
            expiration: decoded['UTCTime'],
            countryName: decoded['countryName-X520'],
            encryptionType: decoded['encryptionAlgo'],
            locality: decoded['localityName-X520'],
            organization: decoded['organizationName-X520'],
            organizationalUnitName: decoded['organizationalUnitName-X520'],
            stateOrProvinceName: decoded['stateOrProvinceName-X520']
        });
    }catch (error){
        errorHandler(error);
    }
}


async function exportCertificate(id){
    try {
        result = await db.certificates.where({id:id}).toArray();
        if(result.length != 1){
            throw new Error("Unexpected number of results when exporting.");
        }
        download(result[0].certificate, `${result[0].fileName}`, "application/x-x509-ca-cert");
    } catch (error) {
        errorHandler(error);
    }
}