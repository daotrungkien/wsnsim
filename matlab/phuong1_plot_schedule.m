chromosome = "0+ 9.4416 20.6321+ 39.7743 60+ 146.002 180+ 189.807 246.798+ 307.043 420+ 510.371 595.065+ 600.731 673.482+ 689.348 720+ 780 813.067+ 822.79 835.731+ 870.413 912.041+ 935.986 1014.75+ 1020 1029.96+ 1036.66 1080+ 1090.49 1098.77+ 1140 1145.79+ 1211.96 1301.18+ 1380 1440+";
chromosome = chromosome + ";0+ 60+ 120 180 240+ 300 360 420+ 480 540 600 660+ 720+ 780 840+ 900 960 1020+ 1080+ 1140+ 1200 1260 1320+ 1380 1440";

nodes = strsplit(chromosome, ';');

figure

for ni = 1:length(nodes)
    arr = split(nodes(ni));
    arr = arr(strlength(arr) > 0);

    xa = [];
    ya = [];
    for i = 1 : length(arr)
        ai = arr{i};

        if ai(end) == '+'
            y_next = 1;
            x_next = str2double(ai(1 : end-1));
        else
            y_next = 0;
            x_next = str2double(ai);
        end

        if i > 1
            xa = [xa x x_next];
            ya = [ya y y];
        end

        y = y_next;
        x = x_next;
    end

    subplot(length(nodes), 1, ni);
    plot(xa , ya, 'LineWidth', 2);
    box on
    grid on
    xlim([0 1440])
    ylim([-0.2 1.2])
    set(gca, 'YTick', [])

    if length(nodes) > 1
        ylabel(sprintf('Node %d schedule', ni));
    else
        ylabel('On/off schedule');
    end
    
    if ni < length(nodes)
        set(gca, 'XTickLabel', [])
    else
        xlabel('Time (h)');
    end
end

