// Función para obtener los datos de la API del backend


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

    // Extraer datos para los gráficos
    const timestamps = data.map(d => new Date(d.timestamp * 1000).toLocaleTimeString());
    const temperatures = data.map(d => d.temperature.toFixed(2));
    const luz = data.map(d => d.luz.toFixed(2));
    const humAir = data.map(d => d.humidity_air.toFixed(2));
    const humSoil = data.map(d => d.humidity_soil.toFixed(2));

    // Mostrar el último valor en vivo
    const latestData = data[data.length - 1];
    document.getElementById('temp-value').innerText = `${latestData.temperature.toFixed(1)} °C`;
    document.getElementById('light-value').innerText = `${latestData.luz.toFixed(1)} luz`;
    document.getElementById('hum-air-value').innerText = `${latestData.humidity_air.toFixed(1)} %`;
    document.getElementById('hum-soil-value').innerText = `${latestData.humidity_soil.toFixed(1)} %`;

    // Gráfico de Temperatura
    new Chart(document.getElementById('temp-chart'), {
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
    new Chart(document.getElementById('luz-chart'), {
        type: 'line',
        data: {
            labels: timestamps,
            datasets: [{
                label: 'Luz',
                data: luz,
                borderColor: 'red',
                backgroundColor: 'rgba(255, 99, 132, 0.2)',
                borderWidth: 1
            }]
        }
    });

    // Gráfico de Humedad del Ambiente
    new Chart(document.getElementById('hum-air-chart'), {
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
    new Chart(document.getElementById('hum-soil-chart'), {
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

