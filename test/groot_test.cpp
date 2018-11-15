#include "groot_test_base.h"
#include "bt_editor/sidepanel_editor.h"
#include <QAction>

class GrootTest : public GrootTestBase
{
    Q_OBJECT

public:
    GrootTest() {}
    ~GrootTest() {}

private slots:
    void renameTabs();
    void initTestCase();
    void cleanupTestCase();
    void loadFile();
    void undoRedo();
    void testSubtree();
};


void GrootTest::initTestCase()
{
    main_win = new MainWindow(GraphicMode::EDITOR, nullptr);
    main_win->resize(1200, 800);
    main_win->show();
}

void GrootTest::cleanupTestCase()
{
    QApplication::processEvents();
    sleepAndRefresh( 2000 );
    main_win->on_actionClear_triggered();
    main_win->close();
}

void GrootTest::loadFile()
{
    QString file_xml = readFile(":/crossdor_with_subtree.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    QString saved_xml = main_win->saveToXML();

    QVERIFY2( file_xml.simplified() == saved_xml.simplified(),
              "Loaded and saved XMl are not the same" );

    sleepAndRefresh( 500 );
    //-------------------------------
    // Compare AbsBehaviorTree
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );
    auto tree_A1 = getAbstractTree("MainTree");
    auto tree_A2 = getAbstractTree("DoorClosed");

    main_win->loadFromXML( saved_xml );
    auto tree_B1 = getAbstractTree("MainTree");
    auto tree_B2 = getAbstractTree("DoorClosed");

    bool same_maintree   = tree_A1 == tree_B1;
    if( !same_maintree )
    {
        tree_A1.debugPrint();
        tree_B1.debugPrint();
    }
    QVERIFY2( same_maintree, "AbsBehaviorTree comparison fails" );

    bool same_doorclosed = tree_A2 == tree_B2;
    if( !same_doorclosed )
    {
        tree_A2.debugPrint();
        tree_B2.debugPrint();
    }

    QVERIFY2( same_doorclosed, "AbsBehaviorTree comparison fails" );

    //-------------------------------
    // Next: expand the Subtree [DoorClosed]
    auto tree = getAbstractTree("MainTree");

    auto subtree_abs_node = tree.findNode("DoorClosed");
    QVERIFY2(subtree_abs_node, "Can't find node with ID [DoorClosed]");

    {
        auto data_model = subtree_abs_node->corresponding_node->nodeDataModel();
        auto subtree_model = dynamic_cast<SubtreeNodeModel*>(data_model);
        QVERIFY2(subtree_model, "Node [DoorClosed] is not SubtreeNodeModel");
        QTest::mouseClick( subtree_model->expandButton(), Qt::LeftButton );
    }
    sleepAndRefresh( 500 );

    //---------------------------------
    // Expanded tree should save the same file
    QString saved_xml_expanded = main_win->saveToXML();
    QVERIFY2( saved_xml_expanded.simplified() == saved_xml.simplified(),
              "Loaded and saved XMl are not the same" );

    //-------------------------------
    // Next: collapse again Subtree [DoorClosed]
    tree = getAbstractTree("MainTree");

    subtree_abs_node = tree.findNode("DoorClosed");
    QVERIFY2(subtree_abs_node, "Can't find node with ID [DoorClosed]");

    {
        auto data_model = subtree_abs_node->corresponding_node->nodeDataModel();
        auto subtree_model = dynamic_cast<SubtreeNodeModel*>(data_model);
        QVERIFY2(subtree_model, "Node [DoorClosed] is not SubtreeNodeModel");
        QTest::mouseClick( subtree_model->expandButton(), Qt::LeftButton );
    }

    sleepAndRefresh( 500 );
}


void GrootTest::undoRedo()
{
    QString file_xml = readFile(":/show_all.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    //------------------------------------------
    AbsBehaviorTree abs_tree_A, abs_tree_B;
    {
        auto container = main_win->currentTabInfo();
        auto view = container->view();
        abs_tree_A = getAbstractTree();

        sleepAndRefresh( 500 );

        auto pippo_node = abs_tree_A.findNode("Pippo");
        auto gui_node = pippo_node->corresponding_node;
        QPoint pippo_screen_pos = view->mapFromScene(pippo_node->pos);
        const QPoint pos_offset(100,0);

        QTest::mouseClick( view->viewport(), Qt::LeftButton, Qt::NoModifier, pippo_screen_pos );

        QVERIFY2( gui_node->nodeGraphicsObject().isSelected(), "Pippo is not selected");

        auto old_pos = view->mapFromScene( container->scene()->getNodePosition( *gui_node ) );
        testDragObject(view, pippo_screen_pos, pos_offset);
        QPoint new_pos = view->mapFromScene( container->scene()->getNodePosition( *gui_node ) );

        QCOMPARE( old_pos + pos_offset, new_pos);
        sleepAndRefresh( 500 );
    }
    //---------- test undo ----------
    {
        abs_tree_B = getAbstractTree();
        main_win->onUndoInvoked();
        sleepAndRefresh( 500 );

        QCOMPARE( abs_tree_A, getAbstractTree() );
        sleepAndRefresh( 500 );
    }

    {
        main_win->onUndoInvoked();
        sleepAndRefresh( 500 );
        auto empty_abs_tree = getAbstractTree();
        QCOMPARE( empty_abs_tree.nodesCount(), size_t(0) );

        // nothing should happen
        main_win->onUndoInvoked();
        main_win->onUndoInvoked();
        main_win->onUndoInvoked();
    }

    {
        main_win->onRedoInvoked();
        sleepAndRefresh( 500 );
        QCOMPARE( getAbstractTree(), abs_tree_A);

        main_win->onRedoInvoked();
        sleepAndRefresh( 500 );
        QCOMPARE( getAbstractTree(), abs_tree_B);

        // nothing should happen
        main_win->onRedoInvoked();
        main_win->onRedoInvoked();
        main_win->onRedoInvoked();
    }

    {
        auto container = main_win->currentTabInfo();
        auto view = container->view();

        auto prev_tree = getAbstractTree();
        size_t prev_node_count = prev_tree.nodesCount();

        auto node = prev_tree.findNode( "DoSequenceStar" );

        QPoint pos = view->mapFromScene(node->pos);
        testMouseEvent(view, QEvent::MouseButtonDblClick, pos , Qt::LeftButton);
        sleepAndRefresh( 500 );

        QTest::keyPress( view->viewport(), Qt::Key_Delete, Qt::NoModifier );
        sleepAndRefresh( 500 );

        auto smaller_tree = getAbstractTree();
        QCOMPARE( prev_node_count - 4 , smaller_tree.nodesCount() );

        main_win->onUndoInvoked();
        sleepAndRefresh( 500 );

        auto undo_tree = getAbstractTree();
        size_t undo_node_count = main_win->currentTabInfo()->scene()->nodes().size();
        QCOMPARE( prev_node_count , undo_node_count );

        QCOMPARE( prev_tree, undo_tree);
        main_win->onRedoInvoked();
        auto redo_tree = getAbstractTree();
        sleepAndRefresh( 500 );

        QCOMPARE( smaller_tree, redo_tree);
    }
    sleepAndRefresh( 500 );
}

void GrootTest::renameTabs()
{
    QString file_xml = readFile(":/crossdor_with_subtree.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    testMessageBox(1000, TEST_LOCATION(), [&]()
    {
        // Two tabs with same name would exist
        main_win->onTabRenameRequested( 0 , "DoorClosed" );
    });
    testMessageBox(1000, TEST_LOCATION(), [&]()
    {
        // Two tabs with same name would exist
        main_win->onTabRenameRequested( 1 , "MainTree" );
    });

    main_win->onTabRenameRequested( 0 , "MainTree2" );
    main_win->onTabRenameRequested( 1 , "DoorClosed2" );

    QVERIFY( main_win->getTabByName("MainTree") == nullptr);
    QVERIFY( main_win->getTabByName("DoorClosed") == nullptr);

    QVERIFY( main_win->getTabByName("MainTree2") != nullptr);
    QVERIFY( main_win->getTabByName("DoorClosed2") != nullptr);

     sleepAndRefresh( 500 );
}

void GrootTest::testSubtree()
{
    QString file_xml = readFile(":/crossdor_with_subtree.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    auto main_tree   = getAbstractTree("MainTree");
    auto closed_tree = getAbstractTree("DoorClosed");

    auto subtree_abs_node = main_tree.findNode("DoorClosed");
    auto data_model = subtree_abs_node->corresponding_node->nodeDataModel();
    auto subtree_model = dynamic_cast<SubtreeNodeModel*>(data_model);

    QTest::mouseClick( subtree_model->expandButton(), Qt::LeftButton );

    QAbstractButton *button_lock = main_win->findChild<QAbstractButton*>("buttonLock");
    QVERIFY2(button_lock != nullptr, "Can't find the object [buttonLock]");

    QTest::mouseClick( button_lock, Qt::LeftButton );

    QTreeWidget* treeWidget = main_win->findChild<QTreeWidget*>("paletteTreeWidget");
    QVERIFY2(treeWidget != nullptr, "Can't find the object [paletteTreeWidget]");

    auto subtree_items = treeWidget->findItems("DoorClosed", Qt::MatchExactly | Qt::MatchRecursive);
    sleepAndRefresh( 500 );

    QCOMPARE(subtree_items.size(), 1);
    sleepAndRefresh( 500 );

    auto sidepanel_editor = main_win->findChild<SidepanelEditor*>("SidepanelEditor");
    sidepanel_editor->onRemoveModel("DoorClosed", false);
    sleepAndRefresh( 500 );

    auto tree_after_remove = getAbstractTree("MainTree");
    auto fallback_node = tree_after_remove.findNode("root_Fallback");

    QCOMPARE(fallback_node->children_index.size(), size_t(3) );
    QVERIFY2( main_win->getTabByName("DoorClosed") == nullptr, "Tab DoorClosed not deleted");
    sleepAndRefresh( 500 );

    //---------------------------------------
    // create again the subtree

    auto closed_sequence_node = tree_after_remove.findNode("door_closed_sequence");
    auto container = main_win->currentTabInfo();
    // This must fail and create a MessageBox
    testMessageBox(1000, TEST_LOCATION(), [&]()
    {
        container->createSubtree( *closed_sequence_node->corresponding_node, "MainTree" );
    });

    container->createSubtree( *closed_sequence_node->corresponding_node, "DoorClosed" );
    sleepAndRefresh( 500 );

    auto new_main_tree   = getAbstractTree("MainTree");
    auto new_closed_tree = getAbstractTree("DoorClosed");

    bool is_same = main_tree == new_main_tree;
    if( !is_same )
    {
        main_tree.debugPrint();
        new_main_tree.debugPrint();
    }
    QVERIFY2( is_same, "AbsBehaviorTree comparison fails" );

    is_same = closed_tree == new_closed_tree;
    if( !is_same )
    {
        closed_tree.debugPrint();
        new_closed_tree.debugPrint();
    }
    QVERIFY2( is_same, "AbsBehaviorTree comparison fails" );
    sleepAndRefresh( 500 );

}

QTEST_MAIN(GrootTest)

#include "groot_test.moc"