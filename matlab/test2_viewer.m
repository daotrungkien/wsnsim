trial = 1;
path = '..\\Debug\\';


file = fopen([path 'wsnsim-log-test2-nodes.txt']);
if file == -1
    error('Node information file not found!');
end
ndata = textscan(file, '%f %s %f %f %f %s');
fclose(file);

file = fopen(sprintf([path 'wsnsim-log-test2-trial-%d.txt'], trial));
if file == -1
    cdata = [];
else
    cdata = textscan(file, '%f %s %s %s');
    fclose(file);
end


fig = figure;
hold on
box on
xlim([-1 31])
ylim([-1 31])
daspect([1 1 1])
xlabel('X (m)');
ylabel('Y (m)');


node_idx = @(name)  find(strcmp(ndata{2}, name));
scircle = @(x, y, r)  rectangle('Position', [x-r,y-r,2*r,2*r], 'Curvature', [1,1]);
dcircle = @(x, y, r)  rectangle('Position', [x-r,y-r,2*r,2*r], 'Curvature', [1,1], 'LineStyle', '-.');
fcircle = @(x, y, r, color) rectangle('Position', [x-r,y-r,2*r,2*r], 'Curvature', [1,1], 'FaceColor', color);

node_size = .4;
node_ring = .6;
master_node = '1';

ndata{7}(1:length(ndata{1})) = -1;

if ~isempty(cdata)
    tmin = min(cdata{1});
    tmax = max(cdata{1});

    if tmax == tmin
        tmax = tmin + 1;
    end
    
    for i = 1 : length(cdata{1})
        t = cdata{1}(i);
        name = cdata{2}{i};
        idx = node_idx(name);
        ndata{7}(idx) = t - tmin;
    end
end


for i = 1 : length(cdata{1})
    t = cdata{1}(i);
    name = cdata{2}{i};
    from = cdata{3}{i};
    action = cdata{4}{i};

    idx = node_idx(name);
    x = ndata{3}(idx);
    y = ndata{4}(idx);
    
    if ~strcmp(from, '-')
        from_idx = node_idx(from);
        fx = ndata{3}(from_idx);
        fy = ndata{4}(from_idx);
        plot([x fx], [y fy], 'Color', [.8 .8 .8]);
    end
    
    switch action
        case 'measure'
            scircle(x, y, node_ring);
        case 'drop'
            dcircle(x, y, node_ring);
    end
end


for i = 1 : length(ndata{1})
    name = ndata{2}{i};
    x = ndata{3}(i);
    y = ndata{4}(i);
    t = ndata{7}(i);
    
    if strcmp(name, master_node)
        scircle(x, y, node_ring);
    end
    
    if t < 0
        fcircle(x, y, node_size, 'white')
    else
        ct = t/(tmax-tmin);
        fcircle(x, y, node_size, [ct ct ct])
    end
    
    text(x, y+1, name, 'HorizontalAlignment', 'center', 'Color', [.5 .5 .5])
end
