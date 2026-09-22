// =====================================================
// KIMBERLY ACTIVITY 4
// DHT11 FIREBASE MONITOR
// PAULENE VISUAL STYLE
// =====================================================

import { initializeApp } from
    "https://www.gstatic.com/firebasejs/12.7.0/firebase-app.js";

import {
    getDatabase,
    ref,
    onValue
} from
    "https://www.gstatic.com/firebasejs/12.7.0/firebase-database.js";

// =====================================================
// FIREBASE CONFIG - PAULENE PROJECT
// =====================================================

const firebaseConfig = {
    apiKey: "AIzaSyBe3OXujc9yOLKCfk1Gj7jxFevNegiQ-cM",
    authDomain: "consular-5a22d.firebaseapp.com",
    databaseURL: "https://consular-5a22d-default-rtdb.europe-west1.firebasedatabase.app",
    projectId: "consular-5a22d",
    storageBucket: "consular-5a22d.firebasestorage.app",
    messagingSenderId: "992611049520",
    appId: "1:992611049520:web:da6c3755788bb1b2918df5",
    measurementId: "G-9KN2FC76FH"
};

// =====================================================
// INITIALIZE FIREBASE
// =====================================================

const firebaseApp = initializeApp(firebaseConfig);
const database = getDatabase(firebaseApp);

const dataRef = ref(database, "ESP32_Data");

// =====================================================
// GLOBAL VARIABLES
// =====================================================

let allSensorData = {};
let selectedDate = "";
let sensorChart = null;

// =====================================================
// DOM
// =====================================================

const statusDot = document.getElementById("statusDot");
const statusText = document.getElementById("statusText");

const currentTemperature =
    document.getElementById("currentTemperature");

const currentHumidity =
    document.getElementById("currentHumidity");

const graphDate =
    document.getElementById("graphDate");

const historyDate =
    document.getElementById("historyDate");

const historyBody =
    document.getElementById("historyTableBody");

const recordCount =
    document.getElementById("recordCount");

const toggleHistory =
    document.getElementById("toggleHistory");

const historyContent =
    document.getElementById("historyContent");

const message =
    document.getElementById("message");

// =====================================================
// STATUS
// =====================================================

function setStatus(text, online) {

    statusText.textContent = text;

    if (online) {
        statusDot.style.background = "#58a66a";
    } else {
        statusDot.style.background = "#999";
    }
}

// =====================================================
// NUMBER HELPERS
// =====================================================

function toNumber(value) {

    const number = Number(value);

    return Number.isFinite(number)
        ? number
        : null;
}

function formatNumber(value) {

    if (value === null || value === undefined) {
        return "--";
    }

    return Number(value).toFixed(1);
}

// =====================================================
// DATE LIST
// =====================================================

function getDateList(data) {

    return Object.keys(data || {})
        .filter(key =>
            data[key] &&
            typeof data[key] === "object"
        )
        .sort()
        .reverse();
}

// =====================================================
// LATEST READING
// =====================================================

function getLatestReading(data) {

    let latest = null;

    const dates =
        Object.keys(data || {}).sort();

    for (const date of dates) {

        const times =
            Object.keys(data[date] || {}).sort();

        for (const time of times) {

            const reading =
                data[date][time];

            if (
                !reading ||
                typeof reading !== "object"
            ) {
                continue;
            }

            const temperature =
                toNumber(reading.temperature);

            const humidity =
                toNumber(reading.humidity);

            if (
                temperature === null &&
                humidity === null
            ) {
                continue;
            }

            latest = {
                date,
                time,
                temperature,
                humidity
            };
        }
    }

    return latest;
}

// =====================================================
// DATE SELECTS
// =====================================================

function populateDateSelect() {

    const dates = getDateList(allSensorData);

    if (graphDate) {

        graphDate.innerHTML = "";

        if (dates.length === 0) {

            graphDate.add(
                new Option("No dates available", "")
            );

        } else {

            dates.forEach(date => {
                graphDate.add(
                    new Option(date, date)
                );
            });

            graphDate.value =
                selectedDate || dates[0];
        }
    }

    if (historyDate) {

        historyDate.innerHTML = "";

        if (dates.length === 0) {

            historyDate.add(
                new Option("No dates available", "")
            );

        } else {

            dates.forEach(date => {
                historyDate.add(
                    new Option(date, date)
                );
            });

            historyDate.value =
                selectedDate || dates[0];
        }
    }
}

// =====================================================
// CURRENT READING
// =====================================================

function updateCurrentReading() {

    const latest =
        getLatestReading(allSensorData);

    if (!latest) {

        currentTemperature.textContent = "-- °C";
        currentHumidity.textContent = "-- %";

        return;
    }

    currentTemperature.textContent =
        formatNumber(latest.temperature) + " °C";

    currentHumidity.textContent =
        formatNumber(latest.humidity) + " %";
}

// =====================================================
// READINGS FOR DATE
// =====================================================

function getReadingsForDate(date) {

    const result = [];

    if (!date) {
        return result;
    }

    const dayData =
        allSensorData[date];

    if (
        !dayData ||
        typeof dayData !== "object"
    ) {
        return result;
    }

    const times =
        Object.keys(dayData).sort();

    times.forEach(time => {

        const reading =
            dayData[time];

        if (
            !reading ||
            typeof reading !== "object"
        ) {
            return;
        }

        const temperature =
            toNumber(reading.temperature);

        const humidity =
            toNumber(reading.humidity);

        if (
            temperature === null &&
            humidity === null
        ) {
            return;
        }

        result.push({
            time,
            temperature,
            humidity
        });
    });

    return result;
}

// =====================================================
// CHART
// =====================================================

function updateChart() {

    if (typeof Chart === "undefined") {
        console.error("Chart.js is not loaded.");
        return;
    }

    const date =
        graphDate.value || selectedDate;

    const readings =
        getReadingsForDate(date);

    const labels =
        readings.map(item => item.time);

    const temperatures =
        readings.map(item => item.temperature);

    const humidities =
        readings.map(item => item.humidity);

    const canvas =
        document.getElementById("sensorChart");

    if (!canvas) {
        return;
    }

    if (sensorChart) {
        sensorChart.destroy();
        sensorChart = null;
    }

    sensorChart = new Chart(canvas, {

        type: "line",

        data: {

            labels,

            datasets: [

                {
                    label: "Temperature (°C)",
                    data: temperatures,
                    yAxisID: "temperature",
                    tension: 0.3,
                    borderWidth: 2.5,
                    pointRadius: 3,
                    pointHoverRadius: 5,
                    borderColor: "#b96786",
                    backgroundColor: "rgba(185,103,134,0.08)"
                },

                {
                    
                        label: "Humidity (%)",
                        data: humidities,
                        yAxisID: "humidity",
                        tension: 0.3,
                        borderWidth: 2.5,
                        pointRadius: 3,
                        pointHoverRadius: 5,
                        borderColor: "#FFD700",
                        backgroundColor: "rgba(255,215,0,0.08)"
                    }

            ]
        },

        options: {

            responsive: true,
            maintainAspectRatio: false,

            interaction: {
                mode: "index",
                intersect: false
            },

            plugins: {

                legend: {
                    labels: {
                        font: {
                            family: "Arial"
                        }
                    }
                }

            },

            scales: {

                x: {
                    ticks: {
                        maxRotation: 45,
                        minRotation: 0
                    }
                },

                temperature: {

                    type: "linear",
                    position: "left",

                    title: {
                        display: true,
                        text: "Temperature (°C)"
                    }
                },

                humidity: {

                    type: "linear",
                    position: "right",

                    title: {
                        display: true,
                        text: "Humidity (%)"
                    },

                    grid: {
                        drawOnChartArea: false
                    }
                }
            }
        }
    });
}

// =====================================================
// HISTORY
// =====================================================

function updateHistory() {

    if (!historyBody) {
        return;
    }

    const date =
        historyDate.value || selectedDate;

    const readings =
        getReadingsForDate(date);

    historyBody.innerHTML = "";

    if (readings.length === 0) {

        historyBody.innerHTML = `
            <tr>
                <td colspan="3" class="empty">
                    No sensor data available.
                </td>
            </tr>
        `;

        recordCount.textContent = "0";

        return;
    }

    readings
        .slice()
        .reverse()
        .forEach(reading => {

            const row =
                document.createElement("tr");

            const timeCell =
                document.createElement("td");

            const temperatureCell =
                document.createElement("td");

            const humidityCell =
                document.createElement("td");

            timeCell.textContent =
                reading.time;

            temperatureCell.textContent =
                formatNumber(reading.temperature) +
                " °C";

            humidityCell.textContent =
                formatNumber(reading.humidity) +
                " %";

            row.appendChild(timeCell);
            row.appendChild(temperatureCell);
            row.appendChild(humidityCell);

            historyBody.appendChild(row);
        });

    recordCount.textContent =
        String(readings.length);
}

// =====================================================
// DASHBOARD
// =====================================================

function updateDashboard() {

    const dates =
        getDateList(allSensorData);

    if (dates.length === 0) {

        currentTemperature.textContent = "-- °C";
        currentHumidity.textContent = "-- %";

        historyBody.innerHTML = `
            <tr>
                <td colspan="3" class="empty">
                    Waiting for records...
                </td>
            </tr>
        `;

        recordCount.textContent = "0";

        if (sensorChart) {
            sensorChart.destroy();
            sensorChart = null;
        }

        return;
    }

    if (
        !selectedDate ||
        !dates.includes(selectedDate)
    ) {
        selectedDate = dates[0];
    }

    populateDateSelect();
    updateCurrentReading();
    updateChart();
    updateHistory();
}

// =====================================================
// FIREBASE REALTIME LISTENER
// =====================================================

setStatus("CONNECTING", false);

onValue(

    dataRef,

    snapshot => {

        try {

            allSensorData =
                snapshot.val() || {};

            setStatus("FIREBASE ONLINE", true);

            message.textContent =
                "Sensor data updated.";

            updateDashboard();

        } catch (error) {

            console.error(
                "Dashboard update error:",
                error
            );

            setStatus("ERROR", false);

            message.textContent =
                "Unable to update dashboard.";
        }
    },

    error => {

        console.error(
            "Firebase read error:",
            error
        );

        setStatus("OFFLINE", false);

        message.textContent =
            "Firebase connection error.";
    }
);

// =====================================================
// DATE EVENTS
// =====================================================

graphDate.addEventListener("change", () => {

    selectedDate =
        graphDate.value;

    historyDate.value =
        selectedDate;

    updateChart();
    updateHistory();
});

historyDate.addEventListener("change", () => {

    selectedDate =
        historyDate.value;

    graphDate.value =
        selectedDate;

    updateHistory();
    updateChart();
});

// =====================================================
// SHOW / HIDE HISTORY
// =====================================================

toggleHistory.addEventListener("click", () => {

    if (historyContent.classList.contains("hidden")) {

        historyContent.classList.remove("hidden");

        toggleHistory.textContent =
            "HIDE HISTORY";

        updateHistory();

    } else {

        historyContent.classList.add("hidden");

        toggleHistory.textContent =
            "SHOW HISTORY";
    }
});
