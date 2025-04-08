const maximumHIDsPerClient = 6;
const maximumHIDsPerInputPort = 4;
const apiTimeout = 5000;
const controllerLoginMaxDurationMS = (30*60000);

function eventHandler(e) {
    console.log(`Event: ${e}`);
    showToast(e);
}


function errorHandler(e) {
    console.error(`Error: ${e}`);
    showToast(e, "danger");
}


function warningHandler(e) {
    console.warn(`Warning: ${e}`);
    showToast(e, "warn");
}


function showToast(message, type='notification', options = {}) {

    if(document.getElementById("toastContainer") === null){
        const toastContainer = document.createElement('div');
        toastContainer.classList.add("toast-container");
        toastContainer.classList.add("top-0");
        toastContainer.classList.add("end-0");
        toastContainer.classList.add("p-3");
        toastContainer.setAttribute("id", "toastContainer");
        document.body.appendChild(toastContainer);
    }

    const toastContainer = document.getElementById("toastContainer");
    const toastInstance = document.createElement('div');
    toastInstance.classList.add('toast');
    toastInstance.classList.add('align-items-center');
    toastInstance.classList.add('border-0');
    toastInstance.setAttribute("role", "alert");
    toastInstance.setAttribute("aria-live", "assertive");
    toastInstance.setAttribute("aria-atomic", "true");

    switch(type){
        case 'danger':
        case 'error':
            toastInstance.classList.add("text-bg-danger");
            options['autohide'] = false;
            break;
        
        case 'warn':
        case 'warning':
            toastInstance.classList.add("text-bg-warning");
            options['delay'] = 5000;
            break;

        case 'success':
            toastInstance.classList.add("text-bg-success");
            options['delay'] = 5000;
            break;

        default:
            toastInstance.classList.add("text-bg-primary");
            options['delay'] = 2500;
            break;
    }

    toastInner = document.createElement('div');
    toastInner.classList.add('d-flex');

    const toastBody = document.createElement('div');
    toastBody.classList.add('toast-body');
    toastBody.innerText = message;
    toastInner.appendChild(toastBody);

    const toastButton = document.createElement('button');
    toastButton.setAttribute("type", "button");
    toastButton.classList.add('btn-close');
    toastButton.classList.add('btn-close-white');
    toastButton.classList.add('me-2');
    toastButton.classList.add('m-auto');
    toastButton.setAttribute("data-bs-dismiss", "toast");
    toastButton.setAttribute("aria-label", "Close");
    toastInner.appendChild(toastButton);

    toastInstance.appendChild(toastInner);
    toastContainer.appendChild(toastInstance);

    const thisToast = new bootstrap.Toast(toastInstance, options);
    thisToast.show();
}


function loadMenu(menu=null, menuItem=null){
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
        if(menu != null){
            new bootstrap.Collapse(document.getElementById(menu), {toggle: true});
        }

        if(menuItem != null){
            document.getElementById(menuItem).classList.add("activeMenuItem");
        }
    })
    .catch(error => {
        errorHandler("Cannot load menu \n" + error);
    });

    fetch("./version.json")
    .then((response) => {
        if (response.ok)
            return response.json();
        else {
            throw new Error(response.statusText);
        }
    })
    .then((content) => {

        if("ui" in content){
            document.getElementById('uiversion').innerText = content.ui;
        }
    })
    .catch(error => {
    });
}


async function exportConfig(){
    try {
        download(await db.export(), `FireFlyConfig_${Date.now()}.json`, "application/json");
        eventHandler("Export successful!");
    } catch (error) {
        errorHandler(error);
    }
}


async function importConfig(file){
    try {
        await db.delete();
        db = await Dexie.import(file);
        eventHandler("Import successful!");
    } catch (error) {
        errorHandler(error);
    }
}


async function resetDatabase(){
    try {
        await db.delete();
        localStorage.clear();
        eventHandler("Reset complete!");
    } catch (error) {
        errorHandler(error);
    }
}


class controllerLocalStorage{
    constructor(id){
        this._id = parseInt(id);
        this._ip = "";
        this._certificates = [];
        this._firmwareVersion = "";
        this._uiVersion = "";
        this._visualToken = "";
        this._uuid = "";
        this._isAuthenticated = false;
        this._eventLog = [];
        this._errorLog = [];
        this._authenticationTime = null;
        this._lastDeploymentTime = null;
        this._provisioningModeEnabled = false;
        this.retrieve();
    }

    get id(){
        return parseInt(this._id);
    }

    get ip(){
        return this._ip;
    }

    /**
     * @param {string} value
     */
    set ip(value){
        this.retrieve();
        this._ip = value;
        this.save();
    }

    get certificates(){
        return this._certificates;
    }

    /**
     * @param {any[]} value
     */
    set certificates(value){
        this.retrieve();
        this._certificates = value;
        this.save();
    }

    get firmwareVersion(){
        return this._firmwareVersion;
    }

    /**
     * @param {string} value
     */
    set firmwareVersion(value){
        this.retrieve();
        this._firmwareVersion = value.trim();
        this.save();
    }

    get uuid(){
        return this._uuid;
    }

    /**
     * @param {string} value
     */
    set uuid(value){
        this.retrieve();
        this._uuid = value;
        this.save();
    }

    get uiVersion(){
        return this._uiVersion;
    }

    /**
     * @param {string} value
     */
    set uiVersion(value){
        this.retrieve();
        this._uiVersion = value.trim();
        this.save();
    }

    get visualToken(){
        return this._visualToken;
    }

    /**
     * @param {string} value
     */
    set visualToken(value){
        this.retrieve();
        this._visualToken = value.trim();
        this.save();
    }

    get isAuthenticated(){
        return this._isAuthenticated;
    }

    get eventLog(){
        return this._eventLog;
    }

    /**
     * @param {any[]} value
     */
    set eventLog(value){
        this.retrieve();
        this._eventLog = value;
        this.save();
    }

    get errorLog(){
        return this._errorLog;
    }

    /**
     * @param {any[]} value
     */
    set errorLog(value){
        this.retrieve();
        this._errorLog = value;
        this.save();
    }


    get lastDeploymentTime(){
        return this._lastDeploymentTime;
    }

    /**
     * @param {BigInt} value
     */
    set lastDeploymentTime(value){
        this.retrieve();
        this._lastDeploymentTime = value;
        this.save();
    }


    /**
     * @param {Boolean} value
     */
    set provisioningModeEnabled(value){
        this.retrieve();
        this._provisioningModeEnabled = value;
        this.save();
    }


    get provisioningModeEnabled(){
        return this._provisioningModeEnabled;
    }


    deleteCertificate(filename){
        this.retrieve();
        this._certificates = this._certificates.filter(certificate => certificate.file !== filename);
        this.save();
    }


    appendCertificate(filename, size){
        this.retrieve();
        this._certificates.push({"file":filename,"size":size});
        this.save();
    }


    retrieve(){
        const record = JSON.parse(localStorage.getItem("controller_" + this._id));

        if(record == null){
            return;
        }

        if("_uuid" in record){
            this._uuid = record._uuid;
        }

        if("_ip" in record){
            this._ip = record._ip;
        }

        if("_visualToken" in record){
            this._visualToken = record._visualToken;
        }

        if("_errorLog" in record){
            this._errorLog = record._errorLog;
        }

        if("_eventLog" in record){
            this._eventLog = record._eventLog;
        }

        if("_isAuthenticated" in record){
            this._isAuthenticated = record._isAuthenticated;
        }

        if("_certificates" in record){
            this._certificates = record._certificates;
        }

        if("_firmwareVersion" in record){
            this._firmwareVersion = record._firmwareVersion;
        }

        if("_uiVersion" in record){
            this._uiVersion = record._uiVersion;
        }

        if("_authenticationTime" in record){
            this._authenticationTime = record._authenticationTime;
        }

        if("_lastDeploymentTime" in record){
            this._lastDeploymentTime = record._lastDeploymentTime;
        }else{
            this._lastDeploymentTime = 0;
        }

        if("_provisioningModeEnabled" in record){
            this._provisioningModeEnabled = record._provisioningModeEnabled;
        }else{
            this._provisioningModeEnabled = false;
        }

        if(this._isAuthenticated){

            if(controllerLoginMaxDurationMS - (new Date() - new Date(this._authenticationTime).getTime()) < 0){
                this._certificates = [];
                this._firmwareVersion = "";
                this._uiVersion = "";
                this._visualToken = "";
                this._uuid = "";
                this._isAuthenticated = false;
                this._authenticationTime = null;
                this._eventLog = [];
                this._errorLog = [];
                this._provisioningModeEnabled = false;
                this.save();
            }
        }
    }

    save(){
        localStorage.setItem("controller_" + this._id, JSON.stringify(this));
    }

    delete(){
        localStorage.removeItem("controller_" + this._id);
    }

    async authenticate(){

        this.retrieve();

        if(this.ip == ""){
            throw new Error(`IP address not set.`);
        }

        if(this.visualToken == ""){
            throw new Error(`Visual token not set.`);
        }

        var response = await fetch(`http://${this.ip}/auth`, {
            signal: AbortSignal.timeout(apiTimeout), 
            method: 'POST',
            headers: {
                "visual-token":this.visualToken
            }
        });

        switch(response.status){
            case 204:
                this._isAuthenticated = true;
                this._authenticationTime = new Date().getTime();
                this.save();
                break;

            case 401:
                this._isAuthenticated = false;
                this.visualToken = "";
                this.save();
                throw new Error(`Authentication failure (${response.status})`);
                break;

            default:
                this._isAuthenticated = false;
                this.visualToken = "";
                this.save();
                throw new Error(`Authentication failure (${response.status})`);
                break;
        }
    }

    logout(){
        this.retrieve();
        this._certificates = [];
        this._firmwareVersion = "";
        this._uiVersion = "";
        this._visualToken = "";
        this._uuid;
        this._isAuthenticated = false;
        this._authenticationTime = null;
        this._eventLog = [];
        this._errorLog = [];
        this.save();
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


async function checkIfInUse_breaker(id){

    const breakers = await db.breakers.where('id').equals(id).toArray();

    if(breakers.length == 0){
        return false;
    }

    await Promise.all(breakers.map(async breaker => {
        [breaker.id] = await Promise.all([
            db.circuits.where('breaker').equals(breaker.id).first()
        ])
    }));

    if(breakers[0].id == undefined){
        return false;
    }
    else{
        return true;
    }
}


async function checkIfInUse_circuit_icon(id){

    const icons = await db.circuit_icons.where('id').equals(id).toArray();

    if(icons.length == 0){
        return false;
    }

    await Promise.all(icons.map(async icon => {
        [icon.circuit] = await Promise.all([
            db.circuits.where('icon').equals(icon.id).first()
        ])
    }));

    if(icons[0].circuit == undefined){
        return false;
    }
    else{
        return true;
    }
}


async function checkIfInUse_area(id){

    const areas = await db.areas.where('id').equals(id).toArray();

    if(areas.length == 0){
        return false;
    }

    await Promise.all(areas.map(async area => {
        [area.circuit] = await Promise.all([
            db.circuits.where('area').equals(area.id).first()
        ])
    }));

    if(areas[0].circuit == undefined){
        return false;
    }
    else{
        return true;
    }
}


async function checkIfInUse_circuit(id){

    assignedCircuits = [];

    controllers = await db.controllers.toArray();

    controllers.forEach((controller) => {
        for (const [key, value] of Object.entries(controller.outputs)) {
            assignedCircuits.push(value);
        }
    });

    if(assignedCircuits.includes(id)){
        return true;
    }

    clients = await db.clients.toArray();

    for(var i=0; i < clients.length; i++){
        for(var j=0; j < clients[i].hids.length; j++){
            for(var k=0; k < clients[i].hids[j].actions.length; k++){
                if(clients[i].hids[j].actions[k].circuit == id){
                    return true;
                }
            }
        }
    }

    return false;
}


async function checkIfInUse_controller(id){

    let controller = await db.controllers.where("id").equals(id).first();

    for (const [key, value] of Object.entries(controller.outputs)) {
        return true;
    }

    for (const [key, value] of Object.entries(controller.inputs)) {
        return true;
    }

    return false;
}


async function checkIfInUse_client(id){

    assignedInputs = [];

    controllers = await db.controllers.toArray();

    controllers.forEach((controller) => {
        for (const [key, value] of Object.entries(controller.inputs)) {
            assignedInputs.push(value);
        }
    });

    if(assignedInputs.includes(id)){
        return true;
    }

    let clients = await db.clients.toArray();

    for(var i = 0; i < clients.length; i++){
        if("extends" in clients[i]){
            if(clients[i].extends == id){
                return true;
            }
        }
    }

    return false;
}


async function checkIfInUse_tag(id){

    var clients = await db.clients.toArray();
    var found = false;

    clients.forEach((client)=>{

        client.hids.forEach((hid)=>{
            hid.tags.forEach((tag)=>{
                if(tag == id){
                    found = true;
                }
            })
        })
    });

    return found;
}


async function checkIfInUse_certificate(id){

    const ota_updates = await db.settings.where("setting").anyOf(["ota_controller","ota_client"]).and(entry=> entry.value.certificate == id).toArray();

    if(ota_updates.length == 0){
        return false;
    }

    return true;

}


async function deleteUnusedCustomRelayModels(){

    relay_models = await db.relay_models.where("is_custom").equals("true").toArray();

    await Promise.all(relay_models.map(async relay_model => {
        [relay_model.circuits] = await Promise.all([
            db.circuits.where('relay_model').equals(relay_model.id).toArray()
        ])
    }));

    for(const relay_model of relay_models){

        if(relay_model.circuits.length == 0){
            await db.relay_models.delete(relay_model.id);
        }
    }
}


function randomUUID(){
    if(typeof self.crypto.randomUUID == "function"){
        return self.crypto.randomUUID();
    }
    
    return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'
    .replace(/[xy]/g, function (c) {
        const r = Math.random() * 16 | 0, 
            v = c == 'x' ? r : (r & 0x3 | 0x8);
        return v.toString(16);
    });
}


function isValidUUID(value){
    var regex =/^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/;
    return regex.test(value);
}


function isValidIPv4(value){
    var regex = /^((25[0-5]|2[0-4][0-9]|[0-1]?[0-9]?[0-9])\.){3}(25[0-5]|2[0-4][0-9]|[0-1]?[0-9]?[0-9])(?::\d{0,4})?$/;
    return regex.test(value);
}


function isValidDomainName(value){
    var regex = /^(((?!-))(xn--|_)?[a-z0-9-]{0,61}[a-z0-9]{1,1}\.)*(xn--)?([a-z0-9][a-z0-9\-]{0,60}|[a-z0-9-]{1,30}\.[a-z]{2,})(?::\d{0,4})??$/;
    return regex.test(value);
}


function isValidURL(value){
    var regex = /^[-a-zA-Z0-9@:%._\+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b(?:[-a-zA-Z0-9()@:%_\+.~#?&//=]*)$/;
    return regex.test(value);
}


function isValidMacAddress(value){
    var regex = /^(?:[0-9A-Fa-f]{2}[:-]){5}(?:[0-9A-Fa-f]{2})$/;
    return regex.test(value);
}


function isValidClientName(value){
    var regex = /^[A-Za-z0-9~!@#$%^&*()_\+-=|]{1,8}$/;
    return regex.test(value);
}


function isValidCircuitName(value){
    var regex = /^[A-Za-z0-9~!@#$%^&*()_\+-=|]{1,8}$/;
    return regex.test(value);
}


function uuidEditFieldRealtimeValidation(element, type){
    uuidElement = document.getElementById(element);

    switch(type){
        case "innerHTML":
            value = uuidElement.innerHTML;
            break;
        case "innerText":
            value = uuidElement.innerText;
            break;
        case "value":
            value = uuidElement.value;
    }

    if(isValidUUID(value)){
        uuidElement.classList.remove("is-invalid");
        uuidElement.classList.add("is-valid");
    }else{
        uuidElement.classList.add("is-invalid");
        uuidElement.classList.remove("is-valid");
    }
}


function macEditFieldRealtimeValidation(element, type){
    macElement = document.getElementById(element);

    switch(type){
        case "innerHTML":
            value = macElement.innerHTML;
            break;
        case "innerText":
            value = macElement.innerText;
            break;
        case "value":
            value = macElement.value;
    }

    if(isValidMacAddress(value)){
        macElement.classList.remove("is-invalid");
        macElement.classList.add("is-valid");
    }else{
        macElement.classList.add("is-invalid");
        macElement.classList.remove("is-valid");
    }
}


function move_array(theArray, from, to) {
    let numberOfDeletedElm = 1;

    const elm = theArray.splice(from, numberOfDeletedElm)[0];

    numberOfDeletedElm = 0;

    theArray.splice(to, numberOfDeletedElm, elm);
    return theArray;
}


async function getExtendedClients(){

    let extendedClients = [];

    clientList =  await db.clients.orderBy('name').toArray()

    clientList.forEach((client)=>{

        if(typeof client.extends != "undefined"){
            extendedClients.push(client.extends);
        }
    });

    return extendedClients;

}


async function checkIOConfiguration(){

    var controllers = await db.controllers.toArray()
    
    controllers.forEach((controller) =>{
        getControllerPOSTPayload(controller.id);
    });
}


async function getControllerPOSTPayload(id, returnOnlyErrors=false){

    const maximumLength_name = 20;
    const maximumLength_area = 20;
    const maximumLength_id = 8;
    const maximumLength_icon = 64;
    let errorList = [];

    let extendedClients = await getExtendedClients();

    var payload = await db.controllers.where("id").equals(id).toArray();

    await Promise.all(payload.map(async controller => {
        [controller.area] = await Promise.all([
            db.areas.where('id').equals(controller.area).first()
        ]), 

        await Promise.all(Object.entries(controller.inputs).map(async client => {
            [controller.inputs[client.at(0)]] = await Promise.all([
                db.clients.where('id').equals(client.at(1)).first()
            ])
        })),
        await Promise.all(Object.entries(controller.outputs).map(async circuit => {
            [controller.outputs[circuit.at(0)]] = await Promise.all([
                db.circuits.where('id').equals(circuit.at(1)).first()
            ])
        })),

        await Promise.all(Object.entries(controller.outputs).map(async circuit => {
            [controller.outputs[circuit.at(0)].area] = await Promise.all([
                db.areas.where('id').equals(circuit.at(1).area).first()
            ])
        })),
        await Promise.all(Object.entries(controller.outputs).map(async circuit => {
            [controller.outputs[circuit.at(0)].relay_model] = await Promise.all([
                db.relay_models.where('id').equals(circuit.at(1).relay_model).first()
            ])
        })),
        await Promise.all(Object.entries(controller.outputs).map(async circuit => {
            [controller.outputs[circuit.at(0)].icon] = await Promise.all([
                db.circuit_icons.where('id').equals(circuit.at(1).icon).first()
            ])
        }))
    }));

    payload[0].name = payload[0].name.trim().substring(0,maximumLength_name);
    payload[0].area = payload[0].area.name.trim().substring(0,maximumLength_area);
    payload[0].ports = payload[0].inputs;

    for(const [portNumber, client] of Object.entries(payload[0].ports)){

        client.channels = {};

        for(let i=0; i<client.hids.length; i++){
            client.channels[i+1] = {};

            if(extendedClients.includes(client.id)){
                client.channels[i+1]['offset'] = maximumHIDsPerInputPort;
            }

            if(client.hids[i].switch_type != "NORMALLY_OPEN"){
                client.channels[i+1]['type'] = client.hids[i].switch_type;
            }

            if(client.hids[i].enabled != true){
                client.channels[i+1]['enabled'] = false;
            }

            client.hids[i].actions.forEach(async (action) => {
                var matched = false;
                for(const [outputPort, circuit] of Object.entries(payload[0].outputs)){
                    if(circuit.id == action.circuit){
                        if("actions" in client.channels[i+1] == false){
                            client.channels[i+1]['actions'] = [];
                        }

                        if(action.change_state == "SHORT"){
                            delete action.change_state;
                        }

                        action['output'] = parseInt(outputPort);

                        for(const [field, value] of Object.entries(action)){
                            const permittedFields = ["change_state","action","output"];
                            if(permittedFields.includes(field) == false){
                                delete action[field];
                            }
                        }

                        matched = true
                        client.channels[i+1]['actions'].push(action);
                    }
                }

                if(!matched){
                    var requestedCircuit = await db.circuits.where("id").equals(action.circuit).first();
                    requestedCircuit.area = await db.areas.where("id").equals(requestedCircuit.area).first();
                    errorList.push(`Client '${client.id}' channel ${i+1} has an invalid action for circuit "${requestedCircuit.area.name} ${requestedCircuit.description} (${requestedCircuit.name})."\n\nThe client is assigned to controller '${payload[0].name}', but the circuit is not assigned to controller '${payload[0].name}'.\n\nThe action will be ignored.`);
                }
            });
        }

        client.id = client.name.trim().substring(0,maximumLength_id);
        client.name = client.description.trim().substring(0,maximumLength_name);

        for(const [field, value] of Object.entries(payload[0].ports[portNumber])){
            const permittedFields = ["name","id","channels"];
            if(permittedFields.includes(field) == false){
                delete payload[0].ports[portNumber][field];
            }
        }
    }

    for(const [outputNumber, circuit] of Object.entries(payload[0].outputs)){
        circuit.id = circuit.name.trim().substring(0,maximumLength_id);
        circuit.name = circuit.description.trim().substring(0,maximumLength_name);
        circuit.area = circuit.area.name.trim().substring(0,maximumLength_area);
        circuit.icon = circuit.icon.icon.trim().substring(0,maximumLength_icon);

        if(circuit.relay_model.type != "BINARY"){
            circuit.type = circuit.relay_model.type;
        }

        if(circuit.enabled == true){
            delete circuit.enabled;
        }
        
        for(const [field, value] of Object.entries(payload[0].outputs[outputNumber])){
            const permittedFields = ["name","id","area","icon","type","enabled"];
            if(permittedFields.includes(field) == false){
                delete payload[0].outputs[outputNumber][field];
            }
        }
    }

    mqtt = await db.settings.where({'setting':'mqtt'}).first();
    
    if(typeof mqtt == "undefined"){
        throw new Error("MQTT has not been configured.")
    }

    payload[0].mqtt = mqtt.value;

    for(const [field, value] of Object.entries(payload[0].mqtt)){
        const permittedFields = ["host","port", "username","password"];
        if(permittedFields.includes(field) == false){
            delete payload[0].mqtt[field];
        }
    }

    ota = await getOTAConfiguration("controller");

    if(Object.keys(ota).length > 0){
        payload[0].ota = ota;

        for(const [field, value] of Object.entries(payload[0].ota)){
            const permittedFields = ["certificate","url"];
            if(permittedFields.includes(field) == false){
                delete payload[0].ota[field];
            }
        }
    }

    for(const [field, value] of Object.entries(payload[0])){
        const permittedFields = ["name","area","ports","outputs", "ota", "mqtt"];
        if(permittedFields.includes(field) == false){
            delete payload[0][field];
        }
    }

    
    if(!returnOnlyErrors){
        return payload[0];
    }else{
        return errorList;
    }
    
}


async function getClientPOSTPayload(id){

    const maximumLength_area = 20;
    const maximumLength_id = 8;
    const maximumLength_color = 20;
    const maximumLength_tag = 20;

    var client = await db.clients.where("id").equals(id).first();

    if(typeof client.extends != "undefined"){
        let extendedClient = await db.clients.where("id").equals(client.extends).first();

        for(var i = 0; i < extendedClient.hids.length; i++){
            client.hids.push(extendedClient.hids[i]);
        }
    }

    client.id = client.name.trim().substring(0, maximumLength_id);

    client.area = await db.areas.where("id").equals(client.area).first();
    client.area = client.area.name.trim().substring(0, maximumLength_area);

    await Promise.all(client.hids.map(async hid => {
        if(typeof hid.color == "undefined"){
            return;
        }

        [hid.color] = await Promise.all([
            db.colors.where('id').equals(hid.color).first()
        ])
    }));

    hids = client.hids;
    client.hids = {};
    
    for(var i=0; i < hids.length; i++){
        client.hids[i+1] = {};
        if(typeof hids[i].color != "undefined"){
            client.hids[i+1]['color'] = hids[i].color.name.trim().substring(0, maximumLength_color);
        }
        
        if(hids[i].tags.length > 0){
            client.hids[i+1]['tags'] = [];

            for(var j=0; j<hids[i].tags.length; j++){
                const tag = await db.tags.where('id').equals(hids[i].tags[j]).first();
                client.hids[i+1]['tags'].push(tag.name.trim().substring(0, maximumLength_tag));
            }
        }
    }

    wifi = await db.settings.where({'setting':'wifi'}).first();
   
    if(typeof wifi == "undefined"){
        throw new Error("WiFi has not been configured.")
    }

    client.wifi = wifi.value;

    for(const [field, value] of Object.entries(client.wifi)){
        const permittedFields = ["ssid","password"];
        if(permittedFields.includes(field) == false){
            delete client.wifi[field];
        }
    }

    mqtt = await db.settings.where({'setting':'mqtt'}).first();
    
    if(typeof mqtt == "undefined"){
        throw new Error("MQTT has not been configured.")
    }

    client.mqtt = mqtt.value;

    for(const [field, value] of Object.entries(client.mqtt)){
        const permittedFields = ["host","username","password"];
        if(permittedFields.includes(field) == false){
            delete client.mqtt[field];
        }
    }

    ota = await getOTAConfiguration("client");

    if(Object.keys(ota).length > 0){
        client.ota = ota;

        for(const [field, value] of Object.entries(client.ota)){
            const permittedFields = ["certificate","url"];
            if(permittedFields.includes(field) == false){
                delete client.ota[field];
            }
        }
    }

    for(const [field, value] of Object.entries(client)){
        const permittedFields = ["name","area","hids","mac","ota", "mqtt", "wifi"];
        if(permittedFields.includes(field) == false){
            delete client[field];
        }
    }

    return client;
}


async function getOTAConfiguration(device_type){

    const maximumLength_url = 128;
    const maximumLength_certificate = 31;

    returnValue = {};

    ota = await db.settings.where({'setting':`ota_${device_type}`}).first();
    
    if(typeof ota == "undefined"){
        return returnValue;
    }

    if(ota.value.enabled == false){
        return returnValue;
    }

    if(ota.value.protocol == "https"){
        certificate = await db.certificates.where('id').equals(ota.value.certificate).first();

        if(typeof certificate == "undefined"){
            throw new Error(`Unknown certificate ID ${ota.value.certificate} in OTA configuration.`);
        }
        returnValue.certificate = certificate.fileName;
    }

    returnValue.url = `${ota.value.protocol}://${ota.value.url}`

    return returnValue;
}