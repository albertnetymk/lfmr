load ./db_list.mat;
% list of mr algo
ebr=1; rc=2; hp=3; dw=4; jvm=5; aoa=6;
gcs = [ebr rc hp dw jvm aoa];
threads = [1 2 4 8 16 31 32 63 64];
elements = [100 200 400 800 1600];
update = [0 20 40 60 80 100];

exclude_rc = 1;
mylegend = {"ebr"; "rc"; "hp"; "encore"; "jvm"; "aoa"};

if exclude_rc == 1
    mylegend(2,:) = [];
end
directory_prefix = 'plots/list'
function myplot(x, y, err, myxlabel, myylabel, xtick, mylegend, mytitle, filename)
    close all
    fullscreen = get(0,'ScreenSize');
    figure(1, 'Position',[0 -50 fullscreen(3) fullscreen(4)]);
    clf;
    y = y/1000;
    err = err/1000;
    set (0, "defaultlinelinewidth", 2);
    if size(x, 2) == 5
        errorbar(...
            x(:, 1), y(:, 1), err(:, 1), '-b', ...
            x(:, 2), y(:, 2), err(:, 2), '-*r', ...
            x(:, 3), y(:, 3), err(:, 3), '-ok', ...
            x(:, 4), y(:, 4), err(:, 4), '-pm', ...
            x(:, 5), y(:, 5), err(:, 5), '-xy' ...
            )
    else
        errorbar(...
            x(:, 1), y(:, 1), err(:, 1), '-b', ...
            x(:, 2), y(:, 2), err(:, 2), '-sg', ...
            x(:, 3), y(:, 3), err(:, 3), '-*r', ...
            x(:, 4), y(:, 4), err(:, 4), '-ok', ...
            x(:, 5), y(:, 5), err(:, 5), '-pm', ...
            x(:, 6), y(:, 6), err(:, 6), '-xy' ...
            )
    end
    % errorbar(x, y, err, ">.r");
    title(mytitle, 'fontsize', 15)
    xlabel(myxlabel, 'fontsize', 15)
    ylabel(myylabel, 'fontsize', 15)
    axis([x(1), x(end)])
    set(gca, 'xtick', xtick)
    set(gca, 'fontsize', 15)
    lh = legend(mylegend);
    % set(lh,'FontSize',15);
    print(1, filename)
end

data_avg = 0;
data_std = 0;
for i = 1:2
    if i == 1
        if exclude_rc == 1
            prefix = strcat(directory_prefix, '/exec_time');
        else
            prefix = strcat(directory_prefix, '/include_rc_exec_time');
        end
        data_avg = db_list_total_avg;
        data_std = db_list_total_std;
        myylabel = 'cpu time per operation (microseconds)';
    else
        if exclude_rc == 1
            prefix = strcat(directory_prefix, '/footprint');
        else
            prefix = strcat(directory_prefix, '/include_rc_footprint');
        end
        data_avg = db_list_footprint_avg;
        data_std = db_list_footprint_std;
        myylabel = 'footprint (kilybytes)';
    end
    for t = threads
        for e = elements
            xtick = update;
            x = repmat(xtick', 1, size(gcs, 2));
            y = x;
            err = x;
            for gc = gcs
                tmp = data_avg(gc, find(t == threads), find(e == elements), :);
                tmp = squeeze(tmp);
                y(:, gc) = tmp;
                tmp = data_std(gc, find(t == threads), find(e == elements), :);
                tmp = squeeze(tmp);
                err(:, gc) = tmp;
            end
            if exclude_rc == 1
                x(:, 2) = [];
                y(:, 2) = [];
                err(:, 2) = [];
            end
            mytitle = ...
              sprintf("lockfree linked list #threads=%d, #elements=%d", t, e);
            myxlabel = 'update percent';
            xtick = update;
            filename = sprintf("%s/list_%d_%d_%s.png", prefix, t, e, 'x');
            myplot(x, y, err, myxlabel, myylabel, xtick, mylegend, mytitle, filename)
        end
    end

    for t = threads
        for u = update
            xtick = elements;
            x = repmat(xtick', 1, size(gcs, 2));
            y = x;
            err = x;
            for gc = gcs
                tmp = data_avg(gc, find(t == threads), :, find(u == update));
                tmp = squeeze(tmp);
                y(:, gc) = tmp;
                tmp = data_std(gc, find(t == threads), :, find(u == update));
                tmp = squeeze(tmp);
                err(:, gc) = tmp;
            end
            if exclude_rc == 1
                x(:, 2) = [];
                y(:, 2) = [];
                err(:, 2) = [];
            end
            mytitle = ...
              sprintf("lockfree linked list #threads=%d, update=%d%%", t, u);
            myxlabel = '#elements';
            filename = sprintf("%s/list_%d_%s_%d.png", prefix, t, 'x', u);
            myplot(x, y, err, myxlabel, myylabel, xtick, mylegend, mytitle, filename)
        end
    end

    for e = elements
        for u = update
            xtick = threads;
            x = repmat(xtick', 1, size(gcs, 2));
            y = x;
            err = x;
            for gc = gcs
                tmp = data_avg(gc, :, find(e == elements), find(u == update));
                tmp = squeeze(tmp);
                y(:, gc) = tmp;
                tmp = data_std(gc, :, find(e == elements), find(u == update));
                tmp = squeeze(tmp);
                err(:, gc) = tmp;
            end
            if exclude_rc == 1
                x(:, 2) = [];
                y(:, 2) = [];
                err(:, 2) = [];
            end
            mytitle = ...
              sprintf("lockfree linked list #elements=%d, update=%d%%", e, u);
            myxlabel = '#threads';
            filename = sprintf("%s/list_%s_%d_%d.png", prefix, 'x', e, u);
            myplot(x, y, err, myxlabel, myylabel, xtick, mylegend, mytitle, filename)
        end
    end
end
return
