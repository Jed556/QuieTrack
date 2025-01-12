// @mui material components
import Grid from "@mui/material/Grid";

// Material Dashboard 2 React components
import MDBox from "components/MDBox";

// Material Dashboard 2 React example components
import { useMaterialUIController } from "context";
import ComplexStatisticsCard from "examples/Cards/StatisticsCards/ComplexStatisticsCard";
import ReportsLineChart from "examples/Charts/LineCharts/ReportsLineChart";
import DashboardLayout from "examples/LayoutContainers/DashboardLayout";
import ReportsBarChart from "examples/Charts/BarCharts/ReportsBarChart";
import DashboardNavbar from "examples/Navbars/DashboardNavbar";
import Footer from "examples/Footer";

// Configs
import configs from "config";

// Data
import reportsBarChartData from "layouts/dashboard/data/reportsBarChartData";

// Dashboard components
import { getDatabase, ref, set, onValue } from "firebase/database";
import OrdersOverview from "layouts/dashboard/components/OrdersOverview";
import Projects from "layouts/dashboard/components/Projects";
import { useState, useEffect } from "react";

function Dashboard() {
    const [controller] = useMaterialUIController();
    const { sidenavColor } = controller;

    const [noiseData, setNoiseData] = useState([]);
    const [noiseLabels, setNoiseLabels] = useState([]);
    const [hourlyAverages, setHourlyAverages] = useState([]);
    const [hourlyLabels, setHourlyLabels] = useState([]);
    const [latestNoiseTime, setLatestNoiseTime] = useState("");

    const numLatestNoiseData = 10;
    const numHourlyData = 7;

    useEffect(() => {
        const db = getDatabase();
        const dataRef = ref(db, "noise/");
        onValue(
            dataRef,
            (snapshot) => {
                const data = snapshot.val();
                if (data) {
                    const noiseArray = Object.values(data);
                    const latestNoiseData = noiseArray.slice(-numLatestNoiseData);
                    const values = latestNoiseData.map((item) => item.value);
                    const times = latestNoiseData.map((item) => new Date(item.time).toLocaleTimeString());
                    setNoiseData(values);
                    setNoiseLabels(times);

                    // Set the latest noise data time
                    if (latestNoiseData.length > 0) {
                        const latestTime = new Date(latestNoiseData[latestNoiseData.length - 1].time);
                        setLatestNoiseTime(latestTime.toLocaleString());
                    }

                    // Calculate hourly averages for the past 8 hours
                    const hourlyData = {};
                    const currentTime = new Date();
                    noiseArray.forEach((item) => {
                        const date = new Date(item.time);
                        const hour = date.getHours();
                        const timeDiff = (currentTime - date) / (1000 * 60 * 60); // Time difference in hours
                        if (timeDiff <= numHourlyData) {
                            if (!hourlyData[hour]) {
                                hourlyData[hour] = [];
                            }
                            hourlyData[hour].push(item.value);
                        }
                    });

                    const averages = [];
                    const labels = [];
                    for (let i = 0; i < numHourlyData; i++) {
                        const hour = (currentTime.getHours() - i + 24) % 24;
                        if (hourlyData[hour]) {
                            const avg =
                                hourlyData[hour].reduce((a, b) => a + b, 0) /
                                hourlyData[hour].length;
                            averages.push(avg);
                            labels.push(`${hour % 12 || 12}${hour >= 12 ? "PM" : "AM"}`);
                        } else {
                            averages.push(0);
                            labels.push(`${hour % 12 || 12}${hour >= 12 ? "PM" : "AM"}`);
                        }
                    }

                    setHourlyAverages(averages.reverse());
                    setHourlyLabels(labels.reverse());
                } else {
                    console.log("No data found"); // Debugging line
                }
            },
            (error) => {
                console.error("Error fetching data:", error); // Debugging line
            }
        );
    }, []);

    console.log(noiseData); // Debugging line

    const noiseChartData = {
        labels: noiseLabels,
        datasets: [
            {
                label: "Noise Level",
                data: noiseData,
                borderColor: "rgba(75,192,192,1)",
                backgroundColor: "rgba(75,192,192,0.2)",
                fill: true,
            },
        ],
    };

    const barChartData = {
        labels: hourlyLabels,
        datasets: [
            {
                label: "Average Noise Level",
                data: hourlyAverages,
                backgroundColor: "rgba(75,192,192,0.2)",
                borderColor: "rgba(75,192,192,1)",
                borderWidth: 1,
            },
        ],
    };

    return (
        <DashboardLayout>
            <DashboardNavbar />
            <MDBox py={3}>
                <Grid container spacing={3}>
                    {/* <Grid item xs={12} md={6} lg={3}>
                        <MDBox mb={1.5}>
                            <ComplexStatisticsCard
                                color="dark"
                                icon="weekend"
                                title="Bookings"
                                count={281}
                                percentage={{
                                    color: "success",
                                    amount: "+55%",
                                    label: "than lask week",
                                }}
                            />
                        </MDBox>
                    </Grid>
                    <Grid item xs={12} md={6} lg={3}>
                        <MDBox mb={1.5}>
                            <ComplexStatisticsCard
                                icon="leaderboard"
                                title="Today's Users"
                                count="2,300"
                                percentage={{
                                    color: "success",
                                    amount: "+3%",
                                    label: "than last month",
                                }}
                            />
                        </MDBox>
                    </Grid>
                    <Grid item xs={12} md={6} lg={3}>
                        <MDBox mb={1.5}>
                            <ComplexStatisticsCard
                                color="success"
                                icon="store"
                                title="Revenue"
                                count="34k"
                                percentage={{
                                    color: "success",
                                    amount: "+1%",
                                    label: "than yesterday",
                                }}
                            />
                        </MDBox>
                    </Grid>
                    <Grid item xs={12} md={6} lg={3}>
                        <MDBox mb={1.5}>
                            <ComplexStatisticsCard
                                color="primary"
                                icon="person_add"
                                title="Followers"
                                count="+91"
                                percentage={{
                                    color: "success",
                                    amount: "",
                                    label: "Just updated",
                                }}
                            />
                        </MDBox>
                    </Grid> */}
                </Grid>
                <MDBox mt={4.5}>
                    <Grid container spacing={3}>
                        <Grid item xs={12} md={6} lg={4}>
                            <MDBox mb={3}>
                                <ReportsBarChart
                                    color="info"
                                    title="Hourly Average Noise Levels"
                                    description="Average noise levels for each hour"
                                    date={latestNoiseTime}
                                    chart={barChartData}
                                />
                            </MDBox>
                        </Grid>
                        <Grid item xs={12} md={6} lg={4}>
                            <MDBox mb={3}>
                                <ReportsLineChart
                                    color="success"
                                    title="Noise Levels"
                                    description="Last 20 noise data points"
                                    date={latestNoiseTime}
                                    chart={noiseChartData}
                                />
                            </MDBox>
                        </Grid>
                        {/* <Grid item xs={12} md={6} lg={4}>
                            <MDBox mb={3}>
                                <ReportsLineChart
                                    color="dark"
                                    title="completed tasks"
                                    description="Last Campaign Performance"
                                    date="just updated"
                                    chart={tasks}
                                />
                            </MDBox>
                        </Grid> */}
                    </Grid>
                </MDBox>
                <MDBox>
                    <Grid container spacing={3}>
                        <Grid item xs={12} md={6} lg={8}>
                            <Projects />
                        </Grid>
                        <Grid item xs={12} md={6} lg={4}>
                            <OrdersOverview />
                        </Grid>
                    </Grid>
                </MDBox>
            </MDBox>
            <Footer company={configs.footer.company} />
        </DashboardLayout>
    );
}

export default Dashboard;
