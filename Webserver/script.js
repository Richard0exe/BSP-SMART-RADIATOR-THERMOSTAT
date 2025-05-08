let temperature = 22;
let minTemp = 8;
let maxTemp = 28;

let radiators = [
{ id: 1, temp: 20, mac:"8C:4F:00:42:02:25", name: "Radiator 1" },
{ id: 2, temp: 20, mac:"8F:4F:00:42:02:28", name: "Radiator 2" },
{ id: 3, temp: 20, mac:"6C:4F:00:42:02:21", name: "Radiator 3" }
];

let editModeId = null; // null = adding, number = editing

function updateTemperatureDisplay() {
    document.getElementById('temp').value = temperature + '°C';
}
function increaseTemp(){
    if(temperature < 28){
    temperature++;
    updateTemperatureDisplay();
    }
}
function decreaseTemp(){
    if(temperature > 8){
    temperature--;
    updateTemperatureDisplay();
    }
}
function setTemperatureFromInput() {
    let input = parseInt(document.getElementById('temp').value);
    if(input >= minTemp && input <= maxTemp) {
        temperature = input;
        updateTemperatureDisplay();
    }
    else {
        document.getElementById('temp').value = temperature + '°C';
    }
}
function sendTemperature(){
    alert(`Temperature ${temperature}°C sent!`)
    radiators.forEach(radiator => {
        radiator.temp = temperature;
        
    });
    renderRadiators();
    fetch(`/set?temperature=${temperature}`)
    .then(response => response.text())  // Read the response body as text
    .then(data => {
        console.log("Response:", data);  // Log the actual response content
    })
    .catch(error => {
        console.error("Error:", error);  // Log any errors that occur
    });
    //need to make that sends to all radiators also
}
function renderRadiators(filteredRadiators = radiators){
    const table = document.querySelector(".radiators-list");

    table.querySelectorAll("tr:nth-child(n+3)").forEach(row => row.remove());

    // Add each radiator row
    filteredRadiators.forEach(radiator => {
    const row = document.createElement("tr");

    const nameCell = document.createElement("td");
    nameCell.textContent = radiator.name;
    nameCell.classList.add("name-cell");

    const tempCell = document.createElement("td");
    tempCell.textContent = radiator.temp + "°C";
    tempCell.classList.add("temp-cell");
    
    const macCell = document.createElement("td");
    macCell.textContent = radiator.mac;
    macCell.classList.add("mac-cell"); // Add the mac-cell class here

    const controlCell = document.createElement("td");
    controlCell.innerHTML = `
        <button onclick="editRadiator(${radiator.id})" title="Edit">
            <img src="edit.svg" alt="Edit" width="28" height="28">
        </button>
        <button onclick="deleteRadiator(${radiator.id})" title="Delete">
            <img src="delete.svg" alt="Delete" width="28" height="28">
        </button>
    `;

    row.appendChild(nameCell);
    row.appendChild(macCell);
    row.appendChild(tempCell);
    row.appendChild(controlCell);
    table.appendChild(row);
});
}

function deleteRadiator(id) {
    radiators = radiators.filter(r => r.id !== id);
    renderRadiators();
}

function onSearch(){
    const query = document.getElementById("searchQuery").value.toLowerCase();
    const filteredRadiators = radiators.filter(r => r.name.toLowerCase().includes(query));
    renderRadiators(filteredRadiators);
}

function editRadiator(id) {
    const radiator = radiators.find(r => r.id === id);
    if (radiator) {
        editModeId = id;
        document.getElementById("newRadiatorName").value = radiator.name;
        document.getElementById("newMac").value = radiator.mac;
        document.getElementById("newTemperature").value = radiator.temp;
        document.querySelector("#addModal h2").textContent = "Edit Radiator";
        document.getElementById("modal-add").textContent = "Edit";
        document.getElementById("addModal").style.display = "block";
    }
}

function synchronize(){
    const message = radiators.map(r => 
    `Name: ${r.name}\nMAC: ${r.mac}\nTemp: ${r.temp}°C`
    ).join('\n\n');
    alert(`Synchronizing with Microcontroller...\n\n${message}`);
    //fetch info from radiators
}
function closeModal() {
    document.getElementById("modal-add").textContent = "Add";
    document.getElementById("addModal").style.display = "none";
}
window.onload = function() {
    updateTemperatureDisplay();
    renderRadiators();

    // Modal setup
const modal = document.getElementById("addModal");
const closeBtn = document.querySelector(".close");

window.addRadiator = function () {
    editModeId = null;
    document.getElementById("newRadiatorName").value = "";
    document.getElementById("newMac").value = "";
    document.getElementById("newTemperature").value = "";
    document.querySelector("#addModal h2").textContent = "Add New Radiator";
    document.getElementById("addModal").style.display = "block";
};

window.submitNewRadiator = function () {
    const newName = document.getElementById("newRadiatorName").value;
    const newMAC = document.getElementById("newMac").value;
    const newTemperature = parseInt(document.getElementById("newTemperature").value);

    const macRegex = /^([0-9A-Fa-f]{2}[:-]?){5}[0-9A-Fa-f]{2}$/;
    if (!macRegex.test(newMAC)) {
        alert("Please enter a valid MAC address.");
        return;
    }

    if (!newName) {
        alert("Please enter a radiator name.");
        return;
    }
    if (!newMAC) {
        alert("Please enter a radiator MAC address.");
        return;
    }

    if(!newTemperature) {
        alert("Please enter temperature.");
        return;
    }

    if(newTemperature < minTemp || newTemperature > maxTemp){
        alert("Please enter valid temperature (8-28°C)");
        return;
    }

    if (editModeId !== null) {
        // Edit existing radiator
        const radiator = radiators.find(r => r.id === editModeId);
        if (radiator) {
            radiator.name = newName;
            radiator.mac = newMAC;
            radiator.temp = newTemperature;
        }
    } else {
        // Add new radiator
        const newId = radiators.length ? Math.max(...radiators.map(r => r.id)) + 1 : 1;
        radiators.push({ id: newId, name: newName, temp: newTemperature, mac: newMAC});
    }

    renderRadiators();
    document.getElementById("addModal").style.display = "none";
    document.getElementById("newRadiatorName").value = "";
    document.getElementById("newMac").value = "";
    document.getElementById("newTemperature").value = "";

    editModeId = null;
};

closeBtn.onclick = function () {
    modal.style.display = "none";
    document.getElementById("modal-add").textContent = "Add";
};

window.onclick = function (event) {
    if (event.target === modal) {
        modal.style.display = "none";
        document.getElementById("modal-add").textContent = "Add";
    }
};
};