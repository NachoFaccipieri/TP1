// Variables globales para los charts
let tempChart, luzChart, humAirChart, humSoilChart;

async function fetchData() {
    try {
        const response = await fetch('http://127.0.0.1:5000/api/data');
        const data = await response.json();
        return data;
    } catch (error) {
        console.error("Error al obtener los datos de la API:", error);
        return [];
    }
}


// Función para actualizar los gráficos
async function updateCharts() {
    const data = await fetchData();
    if (data.length === 0) return;

    // Extraer datos para los gráficos (nombres corregidos)
    const timestamps = data.map(d => new Date(d.timestamp).toLocaleTimeString()).reverse();
    const temperatures = data.map(d => d.temp).reverse();
    const luz = data.map(d => d.luz).reverse();
    const humAir = data.map(d => d.hum_air).reverse();
    const humSoil = data.map(d => d.hum_soil).reverse();

    // Mostrar el último valor en vivo (índice 0 es el más reciente)
    const latestData = data[0];
    document.getElementById('temp-value').innerText = `${latestData.temp.toFixed(1)} °C`;
    document.getElementById('light-value').innerText = `${latestData.luz} %`;
    document.getElementById('hum-air-value').innerText = `${latestData.hum_air} %`;
    document.getElementById('hum-soil-value').innerText = `${latestData.hum_soil} %`;

    // Destruir charts existentes antes de crear nuevos
    if (tempChart) tempChart.destroy();
    if (luzChart) luzChart.destroy();
    if (humAirChart) humAirChart.destroy();
    if (humSoilChart) humSoilChart.destroy();

    // Gráfico de Temperatura
    tempChart = new Chart(document.getElementById('temp-chart'), {
        type: 'line',
        data: {
            labels: timestamps,
            datasets: [{
                label: 'Temperatura (°C)',
                data: temperatures,
                borderColor: 'red',
                backgroundColor: 'rgba(255, 99, 132, 0.2)',
                borderWidth: 1
            }]
        }
    });

    // Gráfico de la luz
    luzChart = new Chart(document.getElementById('light-chart'), {
        type: 'line',
        data: {
            labels: timestamps,
            datasets: [{
                label: 'Luz (%)',
                data: luz,
                borderColor: 'orange',
                backgroundColor: 'rgba(255, 206, 86, 0.2)',
                borderWidth: 1
            }]
        }
    });

    // Gráfico de Humedad del Ambiente
    humAirChart = new Chart(document.getElementById('hum-air-chart'), {
        type: 'line',
        data: {
            labels: timestamps,
            datasets: [{
                label: 'Humedad del Ambiente (%)',
                data: humAir,
                borderColor: 'blue',
                backgroundColor: 'rgba(54, 162, 235, 0.2)',
                borderWidth: 1
            }]
        }
    });

    // Gráfico de Humedad del Suelo
    humSoilChart = new Chart(document.getElementById('hum-soil-chart'), {
        type: 'line',
        data: {
            labels: timestamps,
            datasets: [{
                label: 'Humedad del Suelo (%)',
                data: humSoil,
                borderColor: 'green',
                backgroundColor: 'rgba(75, 192, 192, 0.2)',
                borderWidth: 1
            }]
        }
    });
}

// Expande la card al hacer clic
document.addEventListener('DOMContentLoaded', () => {
    const cards = document.querySelectorAll('.card');
    const overlay = document.getElementById('overlay');

    cards.forEach(card => {
        card.addEventListener('click', () => {
            card.classList.add('expanded');
            overlay.classList.add('active');
        });
    });

    overlay.addEventListener('click', () => {
        document.querySelectorAll('.card.expanded').forEach(card => {
            card.classList.remove('expanded');
        });
        overlay.classList.remove('active');
    });
});

// Llamar a la función al cargar la página
window.onload = updateCharts;

// Actualizar cada 5 segundos
setInterval(updateCharts, 5000);

