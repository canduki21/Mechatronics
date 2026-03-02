%% ===============================
%  Plot Arduino Log Data
%  Voltage (Ch1) + X,Y,Z (Ch2)
% ================================

clc;
clear;
close all;

%% -------- LOAD FILE --------
[file, path] = uigetfile('LOg.TXT');
filename = fullfile(path, file);

% Load assuming tab-separated values
data = readtable(filename, 'Delimiter', '\t');

%% -------- DISPLAY COLUMNS --------
disp("Columns detected:");
disp(data.Properties.VariableNames);

%% -------- EXTRACT DATA --------
% Adjust these names if needed to match your file headers
time = data{:,1};      % Time (seconds)
voltage = data{:,2};   % Channel 1
X = data{:,3};         % Channel 2 - X
Y = data{:,4};         % Channel 2 - Y
Z = data{:,5};         % Channel 2 - Z

%% -------- PLOT --------
figure('Color','w');
hold on;
grid on;

plot(time, voltage, 'LineWidth', 2);
plot(time, X, 'LineWidth', 1.5);
plot(time, Y, 'LineWidth', 1.5);
plot(time, Z, 'LineWidth', 1.5);

xlabel('Time (s)');
ylabel('Signal Value');
title('Voltage and Digital Channels vs Time');

legend('Voltage (Ch.1)', 'X (Ch.2)', 'Y (Ch.2)', 'Z (Ch.2)', ...
       'Location','best');

hold off;

%% -------- OPTIONAL: SEPARATE SUBPLOTS --------
figure('Color','w');

subplot(2,1,1)
plot(time, voltage, 'b', 'LineWidth', 2);
grid on;
xlabel('Time (s)');
ylabel('Voltage (V)');
title('Channel 1: Voltage');

subplot(2,1,2)
plot(time, X, 'r');
hold on;
plot(time, Y, 'g');
plot(time, Z, 'm');
grid on;
xlabel('Time (s)');
ylabel('Digital Data');
title('Channel 2: X, Y, Z');
legend('X','Y','Z');
