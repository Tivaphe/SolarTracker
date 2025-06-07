document.addEventListener('DOMContentLoaded', () => {
    const themeSwitcher = document.getElementById('theme-switcher-container'); // Assuming this is the clickable element
    const themeStylesheet = document.getElementById('theme-stylesheet');
    const themes = ['cyberpunk.css', 'light.css', 'dark.css', 'vhs_y2k.css', 'solarpunk.css']; // Order matters for cycling
    const cssBasePath = 'css/';

    // Load saved theme or default to cyberpunk
    let currentTheme = localStorage.getItem('theme') || themes[0];
    themeStylesheet.setAttribute('href', cssBasePath + currentTheme);

    let currentThemeIndex = themes.indexOf(currentTheme);
    if (currentThemeIndex === -1) { // Should not happen if localStorage is clean
        currentThemeIndex = 0;
        currentTheme = themes[0];
        themeStylesheet.setAttribute('href', cssBasePath + currentTheme);
        localStorage.setItem('theme', currentTheme);
    }

    if (themeSwitcher) {
        themeSwitcher.addEventListener('click', () => {
            currentThemeIndex = (currentThemeIndex + 1) % themes.length;
            currentTheme = themes[currentThemeIndex];
            themeStylesheet.setAttribute('href', cssBasePath + currentTheme);
            localStorage.setItem('theme', currentTheme);
            console.log(`Switched to theme: ${currentTheme}`);
        });
    } else {
        console.error('Theme switcher element not found');
    }

    // Motor Control Button Placeholders
    const activateMotorBtn = document.getElementById('activate-motor-btn');
    const deactivateMotorBtn = document.getElementById('deactivate-motor-btn');

    if (activateMotorBtn) {
        activateMotorBtn.addEventListener('click', () => {
            console.log('Activate Motor button clicked');
            alert('Motor Activation: Placeholder - Not yet implemented.');
            // Later, this will send a command to the server/Arduino
        });
    } else {
        console.error('Activate Motor button not found');
    }

    if (deactivateMotorBtn) {
        deactivateMotorBtn.addEventListener('click', () => {
            console.log('Deactivate Motor button clicked');
            alert('Motor Deactivation: Placeholder - Not yet implemented.');
            // Later, this will send a command to the server/Arduino
        });
    } else {
        console.error('Deactivate Motor button not found');
    }

    // Placeholder Solar Production Chart
    const ctxSolar = document.getElementById('solarProductionChart');

    if (ctxSolar) {
        try {
            new Chart(ctxSolar, {
                type: 'line', // Type of chart
                data: {
                    labels: ['00:00', '01:00', '02:00', '03:00', '04:00', '05:00', '06:00', '07:00', '08:00', '09:00', '10:00', '11:00', '12:00', '13:00', '14:00', '15:00', '16:00', '17:00', '18:00', '19:00', '20:00', '21:00', '22:00', '23:00'],
                    datasets: [{
                        label: 'Solar Production (kWh)',
                        data: [0, 0, 0, 0, 0, 0.1, 0.5, 1.2, 2.5, 3.0, 3.5, 3.8, 4.0, 3.7, 3.2, 2.2, 1.0, 0.4, 0, 0, 0, 0, 0, 0], // Sample data
                        borderColor: '#00ff00', // Cyberpunk green
                        backgroundColor: 'rgba(0, 255, 0, 0.1)',
                        tension: 0.1,
                        fill: true
                    }]
                },
                options: {
                    responsive: true,
                    maintainAspectRatio: false, // Allows chart to fill container height
                    scales: {
                        y: {
                            beginAtZero: true,
                            ticks: {
                                color: '#00ff00' // Y-axis ticks color
                            },
                            grid: {
                                color: 'rgba(0, 255, 0, 0.15)' // Y-axis grid lines color
                            }
                        },
                        x: {
                            ticks: {
                                color: '#00ff00' // X-axis ticks color
                            },
                            grid: {
                                color: 'rgba(0, 255, 0, 0.15)' // X-axis grid lines color
                            }
                        }
                    },
                    plugins: {
                        legend: {
                            labels: {
                                color: '#00ff00' // Legend text color
                            }
                        }
                    }
                }
            });
            console.log('Placeholder solar production chart rendered.');
        } catch (e) {
            console.error('Error rendering chart:', e);
        }
    } else {
        console.error('Solar production chart canvas element not found');
    }
});
